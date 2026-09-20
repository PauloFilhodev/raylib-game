# Projeto de jogo Raylib para fins de estudo na matéria de Algoritmos e Estrutura de Dados I da universidade

## Como utilizar:
- Instalar o MSYS2 UCRT64 com Raylib via pacman:
```pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-gdb mingw-w64-ucrt-x86_64-raylib```
- Crie uma pasta .vscode no diretório base
- Crie os arquivos ```"launch.json", "tasks.json" e "c_cpp_properties.json"``` dentro da .vscode
- Salve os seguintes códigos: 
## ".vscode/tasks.json:"
```
{
    "version": "2.0.0",
    "tasks": [
        {
            "type": "shell",
            "label": "Build Raylib Project",
            "command": "${env:SystemDrive}/msys64/ucrt64/bin/gcc.exe",
            "args": [
                "-Wall",
                "-Wextra",
                "-g3",
                "-I", "${env:SystemDrive}/msys64/ucrt64/include",
                "${workspaceFolder}/src/main.c",
                "-o", "${workspaceFolder}/src/output/main.exe",
                "-L", "${env:SystemDrive}/msys64/ucrt64/lib",
                "-lraylib",
                "-lopengl32",
                "-lgdi32",
                "-lwinmm"
            ],
            "options": {
                "cwd": "${workspaceFolder}/src"
            },
            "problemMatcher": [
                "$gcc"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            }
        }
    ]
}
```
---
## ".vscode/launch.json"
```
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Rodar Jogo (Raylib)",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/src/output/main.exe",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}/src",
            "environment": [
                {
                    "name": "PATH",
                    "value": "${env:SystemDrive}\\msys64\\ucrt64\\bin;${env:PATH}"
                }
            ],
            "externalConsole": false,
            "MIMode": "gdb",
            "miDebuggerPath": "${env:SystemDrive}\\msys64\\ucrt64\\bin\\gdb.exe",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "Build Raylib Project"
        }
    ]
}
```
---
## ".vscode/c_cpp_properties.json"
```
{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**",
                "${env:SystemDrive}/msys64/ucrt64/include"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "compilerPath": "${env:SystemDrive}/msys64/ucrt64/bin/gcc.exe",
            "cStandard": "c11",
            "intelliSenseMode": "windows-gcc-x64"
        }
    ],
    "version": 4
}
```
---
# Ponto de atenção para quem for usar:
Se o você instalou o MSYS2 em outro local (exemplo: D:\ferramentas\msys64), basta alterar a parte "/msys64/ucrt64/" para o caminho onde o MSYS2 está instalado.
