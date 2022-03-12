These are the steps used to locally build under Anaconda for Windows

- Installed VS 2019 Community Edition
- Installed Miniconda x64 for Windows
- Forked and locally cloned the original m17-cxx-demod repo
- Manually applied patches 0001, 0002, 0004 through 0010
- In a fresh Miniconda...
    conda config --add channels conda-forge
    conda create -n M17 vs2019_win-64 cmake ninja pkg-config boost-cpp gtest gmock gtest libcodec2
    conda activate M17
- Copied https://github.com/conda-forge/m17-cxx-demod-feedstock/blob/master/recipe/bld.bat into my repo branch
- Modified the bld.bat
    changed the exit 1 commands to exit /B 1 (so the command shell stayed open)
    added set CPU_COUNT=3
- renamed bld.bat to windows_conda_build.bat
- Executed batch file - and it works! tests passed, and can run the apps

