**Description**

Please include a summary of the change and which issue is fixed or which feature is added.

- [ ] Issue 1 fixed
- [ ] Issue 2 fixed
- [ ] Feature 1 added
- [ ] Feature 2 added

Fixes # (issue)

**How to test this?**

Please describe the tests that you ran to verify your changes. Provide instructions so we can reproduce if there is no integration test added with this PR. Please also list any relevant details for your test configuration

```bash
cmake ..
make -j8
nrnivmodl mod
./bin/nrnivmodl-core mod
./x86_64/special script.py
./x86_64/special-core --tstop=10 --datpath=coredat
```

**Test System**
 - OS: [e.g. Ubuntu 20.04]
 - Compiler: [e.g. PGI 20.9]
 - Version: [e.g. master branch]
 - Backend: [e.g. CPU]

**Use certain branches in CI pipelines.**
<!-- You can steer which versions of CoreNEURON dependencies will be used in
     the various CI pipelines (GitLab, test-as-submodule) here. Expressions are
     of the form PROJ_REF=VALUE, where PROJ is the relevant Spack package name,
     transformed to upper case and with hyphens replaced with underscores.
     REF may be BRANCH, COMMIT or TAG, with exceptions:
      - SPACK_COMMIT and SPACK_TAG are invalid (hpc/gitlab-pipelines limitation)
      - NEURON_COMMIT and NEURON_TAG are invalid (test-as-submodule limitation)
     These values for NEURON, nmodl and Spack are the defaults and are given
     for illustrative purposes; they can safely be removed.
-->
CI_BRANCHES:NEURON_BRANCH=master,NMODL_BRANCH=master,SPACK_BRANCH=develop
