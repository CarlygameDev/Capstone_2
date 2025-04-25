
Advanced Computer Graphics

Project Overview

Capstone 2 aims to build an immersive 3D environment, featuring dynamic weather effects, a skybox, realistic rain, thunder effects, and basic player camera movement.

The main goals of the project include:
- Implementing a skybox with seamless cubemap textures.
- Adding volumetric cloud effects.
- Simulating rain and thunder dynamically.
- Allowing free-look camera movement for exploration.

Features

- Cubemap-based Skybox Rendering
- Dynamic Weather System (Rain, Thunder)
- Free-look Camera Controls (Mouse + Keyboard)
- Textured 3D Environment
- Animated Atmospheric Effects

Technologies Used

- C++
- OpenGL (Modern OpenGL 3.3+ context)
- GLFW – Window and input management
- GLEW – OpenGL Extension Wrangler
- SOIL2 – Texture loading
- GLM – OpenGL Mathematics Library
- Visual Studio 2022 – IDE for development
- Blender – For creating and exporting skybox textures (if needed)

Installation and Setup

1. Clone the repository
   git clone https://github.com/CarlygameDev/Capstone_2.git

2. Install Dependencies
   - Make sure you have GLFW, GLEW, SOIL2, and GLM installed.
   - Set up the project in Visual Studio 2022 (or another C++ IDE).

3. Build the project
   - Open the solution file (.sln) if provided, or manually configure a new project.
   - Ensure the correct include/library directories for OpenGL libraries.

4. Run the executable
   - Once compiled, launch the application to view the 3D scene and interact with it.

Controls

Key/Mouse        Action
---------------  ----------------------------------
W, A, S, D       Move Forward/Left/Backward/Right
Mouse Movement   Rotate Camera View
Esc              Exit the application

Project Structure

Capstone_2/
├── include/        # Header files
├── src/            # Source code (.cpp)
├── textures/       # Skybox and environment textures
├── shaders/        # Vertex and fragment shader programs
├── README.md       # Project documentation
└── LICENSE         # (Optional) License information

Future Improvements

- Add realistic sound effects for thunder.
- Implement particle systems for enhanced rain.
- Optimize rendering pipeline for better performance.
- Expand environment with terrain generation.

License

This project is licensed under the MIT License.

Acknowledgements

- Thanks to Poly Haven for free skybox textures.
- Open-source libraries like GLFW, GLM, and SOIL2.
- Tutorials and resources from OpenGL and C++ communities.

GIF Preview

Insert a short GIF animation here showing camera movement and weather effects.

Feel free to customize this README.md further based on your specific project details!
