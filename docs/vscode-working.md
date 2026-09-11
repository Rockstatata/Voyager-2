Running the exact same Visual Studio project from VS Code

There are actually two ways to do this.

For your current situation, I recommend Method 1.

Method 1 — Keep the .sln/.vcxproj and build it from VS Code

This is the safest approach because:

Visual Studio and VS Code will both build the same project configuration.

You don't need to create CMake yet.

Your existing:

Project1.sln
Project1.vcxproj

remain the source of truth.

Step 1 — Install these VS Code extensions

Open VS Code → Extensions.

Install:

Microsoft C/C++

Search:

C/C++

Publisher:

Microsoft

This gives you:

IntelliSense
C++ debugging
Visual Studio debugger integration

I would also install:

CMake Tools

but we won't actually need CMake yet. It'll become useful later if we decide to make the project portable.

Step 2 — Make sure Visual Studio's C++ build tools are installed

Since your project already builds in Visual Studio, you probably already have them.

You need the Visual Studio workload:

Desktop development with C++

which provides:

MSVC compiler
Windows SDK
MSBuild
link.exe
cl.exe
Step 3 — Open the project using the Visual Studio developer environment

This part matters.

Instead of opening ordinary Command Prompt, search Windows for:

Developer PowerShell for VS 2022

or:

x64 Native Tools Command Prompt for VS 2022

Open it.

Navigate to your solution folder:

cd "C:\path\to\your\Project1"

Then enter:

code .

This opens VS Code with Visual Studio's compiler environment already configured.

Now commands such as:

cl
msbuild

should work.

Try:

cl

You should see something like:

Microsoft (R) C/C++ Optimizing Compiler

And:

msbuild -version

should return an MSBuild version.

Step 4 — Build the Visual Studio solution directly

From the VS Code terminal:

msbuild Project1.sln /m /p:Configuration=Debug /p:Platform=x64

Breaking that down:

Project1.sln

build our solution.

/m

allows parallel compilation.

/p:Configuration=Debug

uses Debug.

/p:Platform=x64

uses 64-bit compilation.

This is effectively the same build Visual Studio performs.

Step 5 — Run the executable

The output is commonly somewhere like:

x64\Debug\Project1.exe

So possibly:

.\x64\Debug\Project1.exe

Depending on the instructor project, it might instead be:

Project1\x64\Debug\Project1.exe

or:

Debug\Project1.exe

Look at Visual Studio:

Project Properties
→ General
→ Output Directory

to determine the exact location.

Important OpenGL issue: working directory

Suppose the code loads a shader like:

"shaders/basic.vert"

The application must run from a location where that relative path makes sense.

Visual Studio might automatically use:

$(ProjectDir)

as the working directory.

VS Code might use something else.

So when we configure VS Code debugging, we'll explicitly set the working directory.

Make VS Code build with Ctrl+Shift+B

Create this folder at your solution root:

.vscode/

Then create:

.vscode/tasks.json

Use:

{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build Project1 - Debug x64",
            "type": "shell",
            "command": "msbuild",
            "args": [
                "${workspaceFolder}\\Project1.sln",
                "/m",
                "/p:Configuration=Debug",
                "/p:Platform=x64"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": "$msCompile"
        }
    ]
}

Then you can simply press:

Ctrl + Shift + B

and VS Code will build the Visual Studio project.

Add debugging with F5

Create:

.vscode/launch.json

Example:

{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Project1 - Debug x64",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "${workspaceFolder}\\x64\\Debug\\Project1.exe",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "console": "integratedTerminal",
            "preLaunchTask": "Build Project1 - Debug x64"
        }
    ]
}

Then:

F5

does:

VS Code
   │
   ├── MSBuild Project1.sln
   │
   ▼
Project1.exe
   │
   ▼
Visual Studio C++ Debugger

You get normal breakpoints too.

You may need to change two paths

Specifically:

"program": "${workspaceFolder}\\x64\\Debug\\Project1.exe"

and:

"cwd": "${workspaceFolder}"

because we don't yet know your exact physical folder layout.

For example, if you have:

SolarProject/
│
├── Project1.sln
│
└── Project1/
    ├── Project1.vcxproj
    ├── shader.vert
    └── shader.frag

and your shader paths assume Project1/, use:

"cwd": "${workspaceFolder}\\Project1"

while the executable may still be:

"program": "${workspaceFolder}\\x64\\Debug\\Project1.exe"
GLFW DLL consideration

Check how GLFW was configured.

If the project links:

glfw3.lib

it's probably statically linked and this isn't an issue.

If it uses:

glfw3dll.lib

then it normally also needs:

glfw3.dll

at runtime.

Usually that DLL should be next to:

Project1.exe

or somewhere on Windows PATH.

If Visual Studio runs correctly but VS Code says something like:

The code execution cannot proceed because glfw3.dll was not found

then that's the issue.

Do not use the VS Code “Run Code” button

This is an important distinction.

If an extension gives you:

▶ Run Code

don't use that for this project.

It may effectively attempt something simplistic such as:

g++ main.cpp

That doesn't understand your:

GLAD
GLFW
OpenGL
include directories
library directories
multiple .cpp files

Our application needs a proper build system.

Use:

MSBuild

or eventually:

CMake

instead.

Method 2 — Eventually use CMake

Long term, I actually like this structure:

Source Code
     │
     ├───────────────┐
     │               │
     ▼               ▼
Visual Studio      VS Code
     │               │
     └──────┬────────┘
            ▼
         CMake
            │
            ▼
         MSVC
            │
            ▼
     VoyagerExplorer.exe

Then the same source project could be opened easily in:

Visual Studio
VS Code
potentially CLion
maybe another Windows machine

with one:

CMakeLists.txt

However, I would not convert to CMake right now.

Why?

Your instructor gave you a working Visual Studio project with its existing:

include paths
library paths
GLAD configuration
GLFW configuration
OpenGL settings

If we immediately introduce CMake, we now have two build systems to debug while also building the solar system.

Unnecessary risk before the 50% lab.

So for now:

             Project1.sln
                   │
         ┌─────────┴─────────┐
         │                   │
         ▼                   ▼
    Visual Studio         VS Code
                            │
                         MSBuild

Same build configuration.

That's exactly what we want.