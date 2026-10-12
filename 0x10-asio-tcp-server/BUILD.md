# Lesson Build

### Build Code without Debug

```bash
cmake -S . -B build
cmake --build build --config Debug --target all -j 10 
```

### Build Code with Debug

```bash
./debug.sh
```

### to run the code

```bash
./build/asio-tcp-server
```
