#include "Demo.h"



Demo::Demo() {

}


Demo::~Demo() {
}



void Demo::Init() {
	BuildShaders();
	BuildDepthMap();
	BuildTexturedCube();
	BuildTexturedPlane();

}

void Demo::DeInit() {
	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &cubeEBO);
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &planeVBO);
	glDeleteBuffers(1, &planeEBO);
	glDeleteBuffers(1, &depthMapFBO);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void Demo::ProcessInput(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}

void Demo::Update(double deltaTime) {
}

void Demo::Render() {
	
	glEnable(GL_DEPTH_TEST);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	
	// Step 1 Render depth of scene to texture
	// ----------------------------------------
	glm::mat4 lightProjection, lightView;
	glm::mat4 lightSpaceMatrix;
	float near_plane = 1.0f, far_plane = 7.5f;
	lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
	lightView = glm::lookAt(glm::vec3(-2.0f, 4.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
	lightSpaceMatrix = lightProjection * lightView;
	// render scene from light's point of view
	UseShader(this->depthmapShader);
	glUniformMatrix4fv(glGetUniformLocation(this->depthmapShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
	glViewport(0, 0, this->SHADOW_WIDTH, this->SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	DrawTexturedCube(this->depthmapShader);
	DrawTexturedPlane(this->depthmapShader);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	

	// Step 2 Render scene normally using generated depth map
	// ------------------------------------------------------
	glViewport(0, 0, this->screenWidth, this->screenHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Pass perspective projection matrix
	UseShader(this->shadowmapShader);
	glm::mat4 projection = glm::perspective(45.0f, (GLfloat)this->screenWidth / (GLfloat)this->screenHeight, 0.1f, 100.0f);
	glUniformMatrix4fv(glGetUniformLocation(this->shadowmapShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	// LookAt camera (position, target/direction, up)
	glm::vec3 cameraPos = glm::vec3(2, 3, 2);
	glm::vec3 cameraFront = glm::vec3(0, 0, 0);
	glm::mat4 view = glm::lookAt(cameraPos, cameraFront, glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(this->shadowmapShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
	
	// Setting Light Attributes
	glUniformMatrix4fv(glGetUniformLocation(this->shadowmapShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
	glUniform3f(glGetUniformLocation(this->shadowmapShader, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	glUniform3f(glGetUniformLocation(this->shadowmapShader, "lightPos"), -2.0f, 4.0f, -1.0f);

	// Configure Shaders
	glUniform1i(glGetUniformLocation(this->shadowmapShader, "diffuseTexture"), 0);
	glUniform1i(glGetUniformLocation(this->shadowmapShader, "shadowMap"), 1);

	// Render floor
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, plane_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	DrawTexturedPlane(this->shadowmapShader);
	
	// Render cube
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cube_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	DrawTexturedCube(this->shadowmapShader);

	glDisable(GL_DEPTH_TEST);
}



void Demo::BuildTexturedCube()
{
	// load image into texture memory
	// ------------------------------
	// Load and create a texture 
	glGenTextures(1, &cube_texture);
	glBindTexture(GL_TEXTURE_2D, cube_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width, height;
	unsigned char* image = SOIL_load_image("crate.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);


	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		// format position, tex coords, normal

		// Bench base
		// Top base
		-0.5,  0.5, 0.5, 0, 0, 0.0f,  1.0f,  0.0f,  // front top left 0
		0.5, 0.5, 0.5, 1, 0, 0.0f,  1.0f,  0.0f,   // front top right 1
		0.5, 0.5, -0.5, 1, 1, 0.0f,  1.0f,  0.0f,   // back top right 2
		-0.5,  0.5, -0.5, 0, 1, 0.0f,  1.0f,  0.0f, // back top left 3

		// Bottom base
		-0.5, 0.4, 0.5, 0, 0, 0.0f,  -1.0f,  0.0f, // front bottom left 4
		0.5, 0.4, 0.5, 1, 0, 0.0f,  -1.0f,  0.0f, // front bottom right 5
		0.5, 0.4, -0.5, 1, 1, 0.0f,  -1.0f,  0.0f, // back bottom right 6
		-0.5, 0.4, -0.5, 0, 1, 0.0f,  -1.0f,  0.0f, // back bottom left 7

		// Front base
		-0.5, 0.4, 0.5, 0, 0, 0.0f,  0.0f,  1.0f, // front top left 8
		0.5, 0.4, 0.5, 1, 0, 0.0f,  0.0f,  1.0f, // front top right 9
		0.5, 0.5, 0.5, 1, 1, 0.0f,  0.0f,  1.0f, // back bottom right 10
		-0.5, 0.5, 0.5, 0, 1, 0.0f,  0.0f,  1.0f, // back bottom left 11

		// Back base
		-0.5, 0.4, -0.5, 0, 0, 0.0f,  0.0f,  -1.0f, // front top left 12
		0.5, 0.4, -0.5, 1, 0, 0.0f,  0.0f,  -1.0f, // front top right 13
		0.5, 0.5, -0.5, 1, 1, 0.0f,  0.0f,  -1.0f, // back bottom right 14
		-0.5, 0.5, -0.5, 0, 1, 0.0f,  0.0f,  -1.0f, // back bottom left 15

		// Right base
		0.5, 0.4, 0.5, 0, 0, 1.0f,  0.0f,  0.0f, // front top left 16
		0.5, 0.4, -0.5, 1, 0, 1.0f,  0.0f,  0.0f, // front top right 17
		0.5, 0.5, -0.5, 1, 1, 1.0f,  0.0f,  0.0f, // back bottom right 18
		0.5, 0.5, 0.5, 0, 1, 1.0f,  0.0f,  0.0f, // back bottom left 19

		// Left base
		-0.5, 0.4, -0.5, 0, 0, -1.0f,  0.0f,  0.0f, // front top left 20
		-0.5, 0.4, 0.5, 1, 0, -1.0f,  0.0f,  0.0f, // front top right 21
		-0.5, 0.5, 0.5, 1, 1, -1.0f,  0.0f,  0.0f, // back bottom right 22
		-0.5, 0.5, -0.5, 0, 1, -1.0f,  0.0f,  0.0f, // back bottom left 23


		// Bench stand
		// Top stand
		-0.5, 0.8, -0.2, 0, 0, 0.0f,  1.0f,  0.0f, // front bottom left 24
		0.5, 0.8, -0.2, 1, 0, 0.0f,  1.0f,  0.0f, // front bottom right 25
		0.5, 0.8, -0.5, 1, 1, 0.0f,  1.0f,  0.0f, // back bottom right 26
		-0.5, 0.8, -0.5, 0, 1, 0.0f,  1.0f,  0.0f, // back bottom left 27

		// Bottom stand
		-0.5, 0.5, -0.2, 0, 0, 0.0f,  -1.0f,  0.0f, // front bottom left 28
		0.5, 0.5, -0.2, 1, 0, 0.0f,  -1.0f,  0.0f, // front bottom right 29
		0.5, 0.5, -0.5, 1, 1, 0.0f,  -1.0f,  0.0f, // back bottom right 30
		-0.5, 0.5, -0.5, 0, 1, 0.0f,  -1.0f,  0.0f, // back bottom left 31

		// Front stand
		-0.5, 0.5, -0.2, 0, 0, 0.0f,  0.0f,  1.0f, // front bottom left 32
		0.5, 0.5, -0.2, 1, 0, 0.0f,  0.0f,  1.0f, // front bottom right 33
		0.5, 0.8, -0.2, 1, 1, 0.0f,  0.0f,  1.0f, // back bottom right 34
		-0.5, 0.8, -0.2, 0, 1, 0.0f,  0.0f,  1.0f, // back bottom left 35

		// Back stand
		-0.5, 0.5, -0.5, 0, 0, 0.0f,  0.0f,  -1.0f, // front bottom left 36
		0.5, 0.5, -0.5, 1, 0, 0.0f,  0.0f,  -1.0f, // front bottom right 37
		0.5, 0.8, -0.5, 1, 1, 0.0f,  0.0f,  -1.0f, // back bottom right 38
		-0.5, 0.8, -0.5, 0, 1, 0.0f,  0.0f,  -1.0f, // back bottom left 39

		// Right stand
		0.5, 0.5, -0.2, 0, 0, 1.0f,  0.0f,  0.0f, // front bottom left 40
		0.5, 0.5, -0.5, 1, 0, 1.0f,  0.0f,  0.0f, // front bottom right 41
		0.5, 0.8, -0.5, 1, 1, 1.0f,  0.0f,  0.0f, // back bottom right 42
		0.5, 0.8, -0.2, 0, 1, 1.0f,  0.0f,  0.0f, // back bottom left 43

		// Left stand
		-0.5, 0.5, -0.5, 0, 0, -1.0f,  0.0f,  0.0f, // front bottom left 44
		-0.5, 0.5, -0.2, 1, 0, -1.0f,  0.0f,  0.0f, // front bottom right 45
		-0.5, 0.8, -0.2, 1, 1, -1.0f,  0.0f,  0.0f, // back bottom right 46
		-0.5, 0.8, -0.5, 0, 1, -1.0f,  0.0f,  0.0f, // back bottom left 47

		// Bench right leg
		// top right leg
		0.4, 0.4, 0.5, 0, 0, 0.0f,  1.0f,  0.0f, // front bottom left 48
		0.5, 0.4, 0.5, 1, 0, 0.0f,  1.0f,  0.0f, // front bottom right 49
		0.5, 0.4, -0.5, 1, 1, 0.0f,  1.0f,  0.0f,  // back bottom right 50
		0.4, 0.4, -0.5, 0, 1, 0.0f,  1.0f,  0.0f, // back bottom left 51

		// bottom right leg
		0.4, 0.2, 0.5, 0, 0, 0.0f,  -1.0f,  0.0f, // front bottom left 52
		0.5, 0.2, 0.5, 1, 0, 0.0f,  -1.0f,  0.0f, // front bottom right 53
		0.5, 0.2, -0.5, 1, 1, 0.0f,  -1.0f,  0.0f, // back bottom right 54
		0.4, 0.2, -0.5, 0, 1, 0.0f,  -1.0f,  0.0f, // back bottom left 55

		// Front right leg
		0.4, 0.2, 0.5, 0, 0, 0.0f,  0.0f,  1.0f, // front bottom left 56
		0.5, 0.2, 0.5, 1, 0, 0.0f,  0.0f,  1.0f, // front bottom right 57
		0.5, 0.4, 0.5, 1, 1, 0.0f,  0.0f,  1.0f, // back bottom right 58
		0.4, 0.4, 0.5, 0, 1, 0.0f,  0.0f,  1.0f, // back bottom left 59

		// Back right leg
		0.4, 0.2, -0.5, 0, 0, 0.0f,  0.0f,  -1.0f, // front bottom left 60
		0.5, 0.2, -0.5, 1, 0, 0.0f,  0.0f,  -1.0f, // front bottom right 61
		0.5, 0.4, -0.5, 1, 1, 0.0f,  0.0f,  -1.0f, // back bottom right 62
		0.4, 0.4, -0.5, 0, 1, 0.0f, 0.0f, -1.0f, // back bottom left 63

		// Right right leg
		0.5, 0.2, 0.5, 0, 0, 1.0f, 0.0f, 0.0f, // front bottom left 64
		0.5, 0.2, -0.5, 1, 0, 1.0f, 0.0f, 0.0f, // front bottom right 65
		0.5, 0.4, -0.5, 1, 1, 1.0f, 0.0f, 0.0f, // back bottom right 66
		0.5, 0.4, 0.5, 0, 1, 1.0f, 0.0f, 0.0f, // back bottom left 67

		// Left right leg
		0.4, 0.2, -0.5, 0, 0, -1.0f, 0.0f, 0.0f, // front bottom left 68
		0.4, 0.2, 0.5, 1, 0, -1.0f, 0.0f, 0.0f, // front bottom right 69
		0.4, 0.4, 0.5, 1, 1, -1.0f, 0.0f, 0.0f, // back bottom right 70
		0.4, 0.4, -0.5, 0, 1, -1.0f, 0.0f, 0.0f, // back bottom left 71

		// Bench left leg
		// top left leg
		-0.5, 0.4, 0.5, 0, 0, 0.0f, 1.0f, 0.0f, // front bottom left 72
		-0.4, 0.4, 0.5, 1, 0, 0.0f, 1.0f, 0.0f, // front bottom right 73
		-0.4, 0.4, -0.5, 1, 1, 0.0f, 1.0f, 0.0f, // back bottom right 74
		-0.5, 0.4, -0.5, 0, 1, 0.0f, 1.0f, 0.0f, // back bottom left 75

		// bottom left leg
		-0.5, 0.2, 0.5, 0, 0, 0.0f, -1.0f, 0.0f, // front bottom left 76
		-0.4, 0.2, 0.5, 1, 0, 0.0f, -1.0f, 0.0f, // front bottom right 77
		-0.4, 0.2, -0.5, 1, 1, 0.0f, -1.0f, 0.0f, // back bottom right 78
		-0.5, 0.2, -0.5, 0, 1, 0.0f, -1.0f, 0.0f, // back bottom left 79

		// Front left leg
		-0.5, 0.2, 0.5, 0, 0, 0.0f, 0.0f, 1.0f, // front bottom left 80
		-0.4, 0.2, 0.5, 1, 0, 0.0f, 0.0f, 1.0f, // front bottom right 81
		-0.4, 0.4, 0.5, 1, 1, 0.0f, 0.0f, 1.0f, // back bottom right 82
		-0.5, 0.4, 0.5, 0, 1, 0.0f, 0.0f, 1.0f, // back bottom left 83

		// Back left leg
		-0.5, 0.2, -0.5, 0, 0, 0.0f, 0.0f, -1.0f, // front bottom left 84
		-0.4, 0.2, -0.5, 1, 0, 0.0f, 0.0f, -1.0f, // front bottom right 85
		-0.4, 0.4, -0.5, 1, 1, 0.0f, 0.0f, -1.0f, // back bottom right 86
		-0.5, 0.4, -0.5, 0, 1, 0.0f, 0.0f, -1.0f, // back bottom left 87

		// Right left leg
		-0.4, 0.2, 0.5, 0, 0, 1.0f, 0.0f, 0.0f, // front bottom left 88
		-0.4, 0.2, -0.5, 1, 0, 1.0f, 0.0f, 0.0f, // front bottom right 89
		-0.4, 0.4, -0.5, 1, 1, 1.0f, 0.0f, 0.0f, // back bottom right 90
		-0.4, 0.4, 0.5, 0, 1, 1.0f, 0.0f, 0.0f, // back bottom left 91

		// Left left leg
		-0.5, 0.2, -0.5, 0, 0, -1.0f, 0.0f, 0.0f, // front bottom left 92
		-0.5, 0.2, 0.5, 1, 0, -1.0f, 0.0f, 0.0f, // front bottom right 93
		-0.5, 0.4, 0.5, 1, 1, -1.0f, 0.0f, 0.0f, // back bottom right 94
		-0.5, 0.4, -0.5, 0, 1, -1.0f, 0.0f, 0.0f, // back bottom left 95
	};

	unsigned int indices[] = {
		// Bench base
		0, 1, 2, 0, 2, 3,  // top face
		4, 5, 6, 4, 6, 7,  // bottom face
		8, 9, 10, 8, 10, 11,  // front face
		12, 13, 14, 12, 14, 15,  // back face
		16, 17, 18, 16, 18, 19, // right face
		20, 21, 22, 20, 22, 23, // left face

		// Bench stand
		24, 25, 26, 24, 26, 27, // top face
		28, 29, 30, 28, 30, 31, // bottom face
		32, 33, 34, 32, 34, 35, // front face
		36, 37, 38, 36, 38, 39, // back face
		40, 41, 42, 40, 42, 43, // right face
		44, 45, 46, 44, 46, 47, // left face

		// Bench right leg
		48, 49, 50, 48, 50, 51, // top face
		52, 53, 54, 52, 54, 55, // bottom face
		56, 57, 58, 56, 58, 59, // front face
		60, 61, 62, 60, 62, 63, // back face
		64, 65, 66, 64, 66, 67, // right face
		68, 69, 70, 68, 70, 71, // left face

		// Bench left leg
		72, 73, 74, 72, 74, 75, // top face
		76, 77, 78, 76, 78, 79, // bottom face
		80, 81, 82, 80, 82, 83, // front face
		84, 85, 86, 84, 86, 87, // back face
		88, 89, 90, 88, 90, 91, // right face
		92, 93, 94, 92, 94, 95 // left face
	};

	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glGenBuffers(1, &cubeEBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(cubeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// define position pointer layout 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// define texcoord pointer layout 1
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// define normal pointer layout 2
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Demo::BuildTexturedPlane()
{
	// Load and create a texture 
	glGenTextures(1, &plane_texture);
	glBindTexture(GL_TEXTURE_2D, plane_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height;
	unsigned char* image = SOIL_load_image("wood.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);
	

	// Build geometry
	GLfloat vertices[] = {
		// format position, tex coords
		// bottom
		-25.0f,	-0.5f, -25.0f,  0,  0, 0.0f,  1.0f,  0.0f,
		25.0f,	-0.5f, -25.0f, 25,  0, 0.0f,  1.0f,  0.0f,
		25.0f,	-0.5f,  25.0f, 25, 25, 0.0f,  1.0f,  0.0f,
		-25.0f,	-0.5f,  25.0f,  0, 25, 0.0f,  1.0f,  0.0f,
	};

	GLuint indices[] = { 0,  2,  1,  0,  3,  2 };

	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glGenBuffers(1, &planeEBO);

	glBindVertexArray(planeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(0);
	// TexCoord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	// Normal attribute
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0); // Unbind VAO
}

void Demo::DrawTexturedCube(GLuint shader)
{
	UseShader(shader);
	glBindVertexArray(cubeVAO);
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0, 0.5f, 0));

	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, 1000, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::DrawTexturedPlane(GLuint shader)
{
	UseShader(shader);
	glBindVertexArray(planeVAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

	glm::mat4 model;
	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::BuildDepthMap() {
	// configure depth map FBO
	// -----------------------
	glGenFramebuffers(1, &depthMapFBO);
	// create depth texture
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, this->SHADOW_WIDTH, this->SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Demo::BuildShaders()
{
	// build and compile our shader program
	// ------------------------------------
	shadowmapShader = BuildShader("shadowMapping.vert", "shadowMapping.frag", nullptr);
	depthmapShader = BuildShader("depthMap.vert", "depthMap.frag", nullptr);
}


int main(int argc, char** argv) {
	RenderEngine &app = Demo();
	app.Start("Shadow Mapping Demo", 800, 600, false, false);
}