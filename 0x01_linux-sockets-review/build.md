# Build Stages

The following commands will create the build directory and generate the project files:
```bash
cmake . -B build 
```

The following commadn will build the project:
```bash
cmake --build build --config Debug --target all -j 10
```
