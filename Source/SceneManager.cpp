///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method loads all the textures for the 3D scene into memory.
 ***********************************************************/
void SceneManager::LoadSceneTextures() {
	// Load textures and assign tags
	bool bReturn = CreateGLTexture("textures/bookcover.jpg", "bookcover_texture");
	if (!bReturn) std::cout << "Failed to load bookcover.jpg!" << std::endl;

	bReturn = CreateGLTexture("textures/bookside.jpg", "bookside_texture");
	if (!bReturn) std::cout << "Failed to load bookside.jpg!" << std::endl;

	bReturn = CreateGLTexture("textures/counter.jpg", "counter_texture");
	if (!bReturn) std::cout << "Failed to load counter.jpg!" << std::endl;

	bReturn = CreateGLTexture("textures/pages.jpg", "pages_texture");
	if (!bReturn) std::cout << "Failed to load pages.jpg!" << std::endl;

	bReturn = CreateGLTexture("textures/wall.jpg", "wall_texture");
	if (!bReturn) std::cout << "Failed to load wall.jpg!" << std::endl;

	bReturn = CreateGLTexture("textures/can.jpg", "can_texture");
	if (!bReturn) std::cout << "Failed to load can.jpg!" << std::endl;

	bReturn = CreateGLTexture("textures/canlid.jpg", "canlid_texture");
	if (!bReturn) std::cout << "Failed to load canlid.jpg!" << std::endl;

	bReturn = CreateGLTexture("textures/apple.jpg", "apple_texture");
	if (!bReturn) std::cout << "Failed to load apple.jpg!" << std::endl;

	bReturn = CreateGLTexture("textures/carbonated.jpg", "carbonated_texture");
	if (!bReturn) std::cout << "Failed to load carnated.jpg!" << std::endl;

	bReturn = CreateGLTexture("textures/foam.jpg", "foam_texture");
	if (!bReturn) std::cout << "Failed to load foam.jpg!" << std::endl;


	// Bind the textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL canMaterial;
	canMaterial.diffuseColor = glm::vec3(40.4f, 0.4f, 0.0f);
	canMaterial.specularColor = glm::vec3(50.7f, 50.7f, 40.6f);
	canMaterial.shininess = 90.0;
	canMaterial.tag = "metal";

	m_objectMaterials.push_back(canMaterial);

	OBJECT_MATERIAL paperMaterial;
	paperMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.3f);
	paperMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	paperMaterial.shininess = 0.1;
	paperMaterial.tag = "paper";

	m_objectMaterials.push_back(paperMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	glassMaterial.specularColor = glm::vec3(21.0f, 16.0f, 11.0f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL CounterMaterial;
	CounterMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	CounterMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	CounterMaterial.shininess = 30.0;
	CounterMaterial.tag = "plate";

	m_objectMaterials.push_back(CounterMaterial);

	OBJECT_MATERIAL backdropMaterial;
	backdropMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.9f);
	backdropMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	backdropMaterial.shininess = 2.0;
	backdropMaterial.tag = "backdrop";

	m_objectMaterials.push_back(backdropMaterial);

	OBJECT_MATERIAL FruitMaterial;
	FruitMaterial.diffuseColor = glm::vec3(0.4f, 0.2f, 0.4f);
	FruitMaterial.specularColor = glm::vec3(0.1f, 0.05f, 0.1f);
	FruitMaterial.shininess = 0.55;
	FruitMaterial.tag = "apple";

	m_objectMaterials.push_back(FruitMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// This enables custom lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Simulated dynamic morning sunlight
	float lightIntensity = 0.8f; // to make quicker day time changes we can adjust this.. 
	                             //Start with a moderate value to simulate early morning

	// Directional light (sunlight)
	m_pShaderManager->setVec3Value("directionalLight.direction", -1.0f, -1.0f, -0.3f); // Low angle for morning light
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.4f * lightIntensity, 0.4f * lightIntensity, 0.35f * lightIntensity);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 1.0f * lightIntensity, 0.85f * lightIntensity, 0.65f * lightIntensity); // Warm morning light
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.9f * lightIntensity, 0.8f * lightIntensity, 0.6f * lightIntensity);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point light (soft bounce light inside the room)
	m_pShaderManager->setVec3Value("pointLights[0].position", -4.0f, 5.0f, 2.0f); // Higher up as ceiling bounce
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.15f, 0.15f, 0.15f);  // Soft bounce
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.25f, 0.25f, 0.3f);   // Soft blue-ish light
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.1f, 0.1f, 0.1f);    // Dim specular
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Point light 1 (another indoor light, warm tone from a light source near the window)
	m_pShaderManager->setVec3Value("pointLights[1].position", 2.0f, 6.0f, -3.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.2f, 0.18f, 0.15f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.45f, 0.4f, 0.35f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.5f, 0.4f, 0.3f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// Light gradually gets brighter throughout the scene; adjust lightIntensity manually in your render loop to simulate changes
}




	

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	//LoadSceneTextures(); <--Left this here because i loaded this twice and it took me 4 hours to find out why i couldnt add more tha 8 textures :')
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();

	//Load new shapes for complex image cup and book base
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTorusMesh();

	// Load all the textures into memory
	LoadSceneTextures();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// -----------------------------------------------------
	// Draw the plane
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(50.0f, 1.0f, 20.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -0.6f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.5, 0, 0, 1);
	SetShaderTexture("counter_texture");//This texture was from actual photo samples and made seemless by myself using bluring at edges :)
	SetTextureUVScale(2.0, 2.0); //Tiled texture to help quality look better
	SetShaderTexture("plate");
	m_basicMeshes->DrawPlaneMesh();

	// -----------------------------------------------------
	// Draw Cylinder Sparkling Bev can (Main shape)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(1.5f, 8.0f, 1.5f);  // Taller cylinder for cup base
	positionXYZ = glm::vec3(-3.0f, 2.0f, 0.0f);  // Adjust the position
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0);
	SetShaderTexture("can_texture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderTexture("glass");
	m_basicMeshes->DrawCylinderMesh();

	// -----------------------------------------------------
	// Draw Cylinder Sparkling Bev can (Top lid)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(1.45f, 0.001f, 1.45f);  // shorter cylinder for cup lid
	positionXYZ = glm::vec3(-3.0f, 10.0f, 0.0f);  // Adjust the position
	SetTransformations(scaleXYZ, 0.0f, 75.0f, 0.0f, positionXYZ);
	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0);
	SetShaderTexture("canlid_texture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderTexture("glass");
	m_basicMeshes->DrawCylinderMesh();

	// -----------------------------------------------------
	// Draw Cylinder 1 (STEM CUP)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(0.25f, 1.0f, 0.25f);  // Taller cylinder for cup stem
	positionXYZ = glm::vec3(-6.0f, 3.4f, 3.0f);  // Adjust the position
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.3);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawCylinderMesh();

	// -----------------------------------------------------
	// Draw Cylinder 2 (BASE FOR CUP BOTTOM)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(1.3f, 1.0f, 1.3f);  // Base cylinder For cup bottom
	positionXYZ = glm::vec3(-6.0f, 2.0f, 3.0f);  // Adjust the position
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.4);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawCylinderMesh();

	// -----------------------------------------------------
	// Draw tapered Cylinder (Top of cup)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(1.4f, 1.5f, 1.4f);  // cylinder tapered to make top smaller than base of cup
	positionXYZ = glm::vec3(-6.0f, 6.3f, 3.0f);  // Adjust the position
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.7);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// -----------------------------------------------------
	// Draw Cylinder (Top middle of cup)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(1.4f, 0.5f, 1.4f);  // cylinder to make top smaller than base of cup
	positionXYZ = glm::vec3(-6.0f, 5.80f, 3.0f);  // Adjust the position
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.7);
	//SetShaderTexture("foam_texture");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawCylinderMesh();

	// -----------------------------------------------------
	// Draw Sphere (cup round)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(1.4f, 1.5f, 1.4f);  // Sphere size
	positionXYZ = glm::vec3(-6.0f, 5.90f, 3.0f);  // Center position
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.7);
	//SetShaderTexture("carbonated_texture");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawSphereMesh();

	// -----------------------------------------------------
	// Draw Cone 1 (Lower base of cup above cylinder for roundness)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(1.3f, 0.5f, 1.3f);  // lower cup cone
	positionXYZ = glm::vec3(-6.0f, 3.0f, 3.0f);  // Adjust the position
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.3);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawConeMesh();

	// -----------------------------------------------------
	// Draw Cone 2 (For top of stem and sphere to smoothen)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(1.0f, -1.0f, 1.0f);  // Smaller cone
	positionXYZ = glm::vec3(-6.0f, 5.0f, 3.0f);  // Adjust the position
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.3);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawConeMesh();

	// -----------------------------------------------------
	// Draw Box (book pages)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(15.0f, 2.0f, 10.0f);  // Box size
	positionXYZ = glm::vec3(-2.0f, 1.0f, 2.5f);  // Adjust the position
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.9f, 0.7f, 0.1f, 1);
	SetShaderTexture("pages_texture");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMesh();
	SetShaderMaterial("paper");

	// -----------------------------------------------------
	// Draw Box (book cover)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(10.5f, 0.25f, 15.5f);  // Box size
	positionXYZ = glm::vec3(-2.0f, 2.0f, 2.5f);  // Adjust the position
	SetTransformations(scaleXYZ, 0.0f, 90.0f, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.9f, 0.7f, 0.1f, 1);
	SetShaderTexture("bookcover_texture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("paper");
	m_basicMeshes->DrawBoxMesh();

	// -----------------------------------------------------
	// Draw Box (book cover bottom)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(10.5f, 0.25f, 15.5f);  // Box size
	positionXYZ = glm::vec3(-2.0f, -0.25f, 2.5f);  // Adjust the position
	SetTransformations(scaleXYZ, 0.0f, 90.0f, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.9f, 0.7f, 0.1f, 1);
	SetShaderTexture("bookcover_texture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("paper");
	m_basicMeshes->DrawBoxMesh();

	// -----------------------------------------------------
	// Draw Box (book spine)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(15.5f, 0.25f, 2.5f);  // Box size
	positionXYZ = glm::vec3(-2.0f, 0.90f, 7.75f);  // Adjust the position
	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	//SetShaderColor(0.9f, 0.7f, 0.1f, 1);
	SetShaderTexture("bookside_texture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("paper");
	m_basicMeshes->DrawBoxMesh();

	// -----------------------------------------------------
	// Draw plane (back drywall)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(50.0f, 0.25f, 30.0f);  // Box size
	positionXYZ = glm::vec3(0.0f, 20.0f, -4.0f);  // Adjust the position
	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	//SetShaderColor(0.9f, 0.7f, 0.1f, 1);
	SetShaderTexture("wall_texture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("backdrop");
	m_basicMeshes->DrawPlaneMesh();

	// -----------------------------------------------------
	// Draw Sphere 2 (Apple)
	// -----------------------------------------------------
	scaleXYZ = glm::vec3(3.0f, 1.6f, 3.0f);  // not perfectly round
	positionXYZ = glm::vec3(1.7f, 3.4f, 3.0f);  // Adjust the position
	SetTransformations(scaleXYZ, -1.0f, 90.0f, -10.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0);
	SetShaderTexture("apple_texture");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("apple");
	m_basicMeshes->DrawSphereMesh();

}
