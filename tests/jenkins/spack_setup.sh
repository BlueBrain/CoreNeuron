#!/bin/bash

# Based on spack_setup.sh from blueconfigs repo

# NOTE: This file shall be sourced so that important variables are avail to other scripts
_THISDIR=$(readlink -f $(dirname $BASH_SOURCE))
source "$_THISDIR/envutils.sh"

echo "
=====================================================================
Preparing spack environment...
====================================================================="

[ "$WORKSPACE" ] || { log_error "Please define WORKSPACE"; return 1; }

export SPACK_INSTALL_PREFIX="${SPACK_INSTALL_PREFIX:-${WORKSPACE}/INSTALL_HOME}"
export SOFTS_DIR_PATH=$SPACK_INSTALL_PREFIX  # Deprecated, but might still be reqd

# Give the use the possibility of running with an existing spack
if [[ "$USE_SYSTEM_SPACK" && "$SPACK_ROOT" ]]; then
    log_warn "Using system spack at $SPACK_ROOT"
    return
fi

BUILD_HOME="${WORKSPACE}/BUILD_HOME"
export SPACK_ROOT="${BUILD_HOME}/spack"
log "SPACK_INSTALL_PREFIX=$SPACK_INSTALL_PREFIX; BUILD_HOME=$BUILD_HOME" DBG

# ENV SETUP

# TODO: /usr/bin was added as a quickfix due to git dependencies probs
export PATH=$SPACK_ROOT/bin/spack:/usr/bin:$PATH

# MODULES
# Use spack only modules. Last one is added by changing MODULEPATH since it might not exist yet
module purge
unset MODULEPATH
source /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
export MODULEPATH=$SPACK_INSTALL_PREFIX/modules/tcl/linux-rhel7-x86_64:$MODULEPATH
PYDEPS_PATH=$BUILD_HOME/pydevpkgs


############################# CLONE/SETUP REPOSITORY #############################

install_spack() (
    set -e
    BASEDIR="$(dirname "$SPACK_ROOT")"
    mkdir -p $BASEDIR && cd $BASEDIR
    rm -rf .spack   # CLEANUP SPACK CONFIGS
    SPACK_REPO=https://github.com/BlueBrain/spack.git
    SPACK_BRANCH=${SPACK_BRANCH:-"develop"}

    log "Installing SPACK. Cloning $SPACK_REPO $SPACK_ROOT --depth 1 -b $SPACK_BRANCH"
    git clone $SPACK_REPO $SPACK_ROOT --depth 1 -b $SPACK_BRANCH
    # Use BBP configs
    mkdir -p $SPACK_ROOT/etc/spack/defaults/linux
    cp /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/*.yaml $SPACK_ROOT/etc/spack/

)


# Use centralized python-dev package set
config_py_deps() (
    set -e
    # create a link to centralized site-packages
    site_pkgs=$($DEVEL_DEPLOY/pyenv/bin/python -m site | grep $DEVEL_DEPLOY | sed "s#[,']##g" )
    log "Configuring Python dependencies (src: $site_pkgs)"
    ln -sf $site_pkgs $PYDEPS_PATH

   external_pkg_tpl='
  ${PKG_NAME}:
    version: [${PKG_VERSION}]
    paths:
      ${PKG_NAME}@${PKG_VERSION}${PKG_VARIANT}: ${PKG_PATH}'
    external_pkg_module_tpl="$external_pkg_tpl"'
    modules:
      ${PKG_NAME}@${PKG_VERSION}${PKG_VARIANT}: ${PKG_MODULE}'

    # mpi4py (work w intel stack)
    echo "$external_pkg_module_tpl" | PKG_NAME=py-mpi4py PKG_VERSION=99 PKG_PATH=. PKG_MODULE=py-mpi4py \
        envsubst >> $SPACK_ROOT/etc/spack/packages.yaml

    for pkg in numpy h5py lazy-property setuptools; do
        echo "$external_pkg_tpl" | PKG_NAME=py-$pkg PKG_VERSION=99 PKG_PATH=pydeps envsubst >> $SPACK_ROOT/etc/spack/packages.yaml
    done
)


spack_setup() (
    set -e
    # Install a Spack environment if needed
    if [ -d $SPACK_ROOT ]; then
        log_warn "Using existing spack at $SPACK_ROOT"
    else
        install_spack
    fi

    # Setup python deps
    [ -e "$PYDEPS_PATH" ] || config_py_deps

    log_ok "Spack environment setup done"
)


spack_setup

