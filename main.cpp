#include <GL/glew.h>    // GLEW
#include <GLFW/glfw3.h> // GLFW
#include <iostream>     // For console output

// Vertex Shader source code
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos; // The position attribute for each vertex
    void main() {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0); // Set the final position of the vertex
    }
)";

// Fragment Shader source code
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor; // The output color of the fragment
    void main() {
        FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f); // Orange color (RGBA) for all pixels
    }
)";

// Function to compile a shader
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type); // Create a new shader object
    glShaderSource(shader, 1, &source, NULL); // Set the shader source code
    glCompileShader(shader); // Compile the shader

    // Check for compilation errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }
    return shader;
}

// Function to link shaders into a program
GLuint createProgram(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram(); // Create a new shader program
    glAttachShader(program, vertexShader); // Attach the vertex shader
    glAttachShader(program, fragmentShader); // Attach the fragment shader
    glLinkProgram(program); // Link the program

    // Check for linking errors
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return 0;
    }
    return program;
}

int main() {
    // 1. Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set OpenGL version (e.g., 3.3 Core Profile)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Use core profile for modern GL

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required for macOS
#endif

    // 2. Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Simple OpenGL Triangle", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // Make the window's context current

    // 3. Initialize GLEW
    // glewExperimental = true; is often recommended to ensure all modern functions are loaded.
    // It's not strictly necessary for this simple example but good practice.
    glewExperimental = GL_TRUE; 
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Print OpenGL version (optional)
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // 4. Build and compile our shader program
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint shaderProgram = createProgram(vertexShader, fragmentShader);

    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 5. Define triangle vertex data
    // Coordinates are in normalized device coordinates (NDC), from -1.0 to +1.0
    float vertices[] = {
        // positions (x, y, z)
         0.0f,  0.5f, 0.0f,  // Top vertex
        -0.5f, -0.5f, 0.0f,  // Bottom-left vertex
         0.5f, -0.5f, 0.0f   // Bottom-right vertex
    };

    // 6. Setup Vertex Array Object (VAO) and Vertex Buffer Object (VBO)
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO); // Generate 1 VAO
    glGenBuffers(1, &VBO);     // Generate 1 VBO

    // Bind the VAO first, then bind and set vertex buffers, and then configure vertex attributes.
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO); // Bind VBO to ARRAY_BUFFER target
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Copy vertex data to VBO

    // Specify how to interpret the vertex data for attribute 'aPos' (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); // Enable the vertex attribute at location 0

    // Unbind the VBO and VAO (optional, but good practice to avoid accidental modification)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Set the viewport (optional, GLFW usually handles this initially)
    // glViewport(0, 0, 800, 600); // For 800x600 window

    // 7. Render loop
    while (!glfwWindowShouldClose(window)) {
        // Input handling (e.g., close window on ESC key)
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // Clear the screen with a specific color
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark grey background
        glClear(GL_COLOR_BUFFER_BIT); // Clear the color buffer

        // Use the shader program
        glUseProgram(shaderProgram);
        // Bind the VAO to draw its configured vertex data
        glBindVertexArray(VAO);
        // Draw the triangle (3 vertices, starting from index 0, type GL_TRIANGLES)
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Swap front and back buffers
        glfwSwapBuffers(window);
        // Poll for and process events
        glfwPollEvents();
    }

    // 8. Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate(); // Terminate GLFW, clearing any allocated resources
    return 0;
}
