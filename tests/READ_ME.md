**WSL/Linux build & test (recommended):**

```bash
./scripts/test.sh
```

**Windows (legacy):**

```powershell
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```
