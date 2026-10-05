# build commands used

* Configure the project (generate build system):
```bash
cmake -S . -B build
```

* Build the project (compile):
```bash
cmake --build build --config Debug --target all -j 10
```