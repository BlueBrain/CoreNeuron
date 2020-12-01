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

**Use certain branches for the SimulationStack CI**

CI_BRANCHES:NEURON_BRANCH=master,
