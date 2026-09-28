# userdata

`CMakeLists.txt` registers `main.c` as an ESP-IDF component:

```cmake
idf_component_register(SRCS "main.c" INCLUDE_DIRS ".")
```

See [`../../README.md`](../../README.md) for the firmware description and build steps.
