## Dependencies

The following tools need to be installed before running the configuration step:

- clang >= 10
- cmake >= 3.18


The following packages must be installed before running the configuration step:

- Vulkan SDK >= 1.2
- xxHash (static library) >= 0.7.0
- GLM 0.9.9.8


## Configuring the development environment

To configure the optional features, modify the `features.cmake` file as needed.


To initialize the build:

    cmake -P scripts/init.cmake


To build the library, samples and tests:

    cmake --build build


To run the tests:

    ctest --output-on-failure

This needs to be run inside `build/tests`.


## Cleaning up

Any `{FILENAME}.in` file will have a corresponding `{FILENAME}` generated next to it. To fully clean the build environment, the following must be deleted:

- `build/`
- Any `{FILENAME}` as mentioned above
- `compile_commands.json`

The `features.cmake` file must also reset to its initial state.
