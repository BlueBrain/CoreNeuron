# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

find_program(SLURM_SRUN_COMMAND srun DOC "Path to the SLURM srun executable")
mark_as_advanced(SLURM_SRUN_COMMAND)

if(SLURM_SRUN_COMMAND)
  set(SLURM_FOUND true)
else()
  set(SLURM_FOUND false )
endif()

