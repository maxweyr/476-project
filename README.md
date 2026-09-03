# Build Instructions

* Uses cmake to compile and needs Visual Studio MSBuild version 17.12.12+1cce7796

`cmake -build .`

or for Release build

`cmake -DCMAKE_BUILD_TYPE=Release ..`

`cmake --build . --config Release`

`.\Release\LibraryGame.exe`