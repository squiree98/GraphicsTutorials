#include "HeightMap.h"
#include <iostream>

using namespace std;

HeightMap::HeightMap(const string& name) {
	int iWidth, iHeight, iChans;
	unsigned char* data = SOIL_load_image(name.c_str(), &iWidth, &iHeight, &iChans, 1);
	if (!data) {
		cout << "Heightmap could not be loaded" << endl;
		return;
	}
	numVertices =			iWidth * iHeight;
	numIndices =			(iWidth - 1) * (iHeight - 1) * 6;
	vertices =				new Vector3[numVertices];
	textureCoords =			new Vector2[numVertices];
	indices =				new GLuint[numIndices];

	Vector3 vertexScale =	Vector3(16.0f, 1.0f, 16.0f);
	Vector2 textureScale =	Vector2(1/16.0f, 1/16.0f);

	// loop through each vertex in height map and translate vertices
	//  and texture coords into 2D arrays
	for (int z = 0; z < iHeight; z++)
	{
		for (int x = 0; x < iWidth; x++)
		{
			// this simply makes sure that when a data item goes off
			// the edge of the 2D array this is represented in 1D array
			int offset = (z * iWidth) + x;
			vertices[offset] = Vector3(x, data[offset], z) * vertexScale;
			textureCoords[offset] = Vector2(x, z) * textureScale;
		}
	}
	// deleted uneeded data
	SOIL_free_image_data(data);

	// creates the 2D plane which terrain will be created on
	int i = 0;
	for (int z = 0; z < iHeight-1; z++)
	{
		for (int x = 0; x < iWidth-1; x++)
		{
			// our corners of square patches
			int a = (z		* iWidth) +	 x;
			int b = (z		* iWidth) + (x+1);
			int c = ((z+1)	* iWidth) + (x+1);
			int d = ((z+1)	* iWidth) +  x;
			// triangle one
			indices[i++] = a;
			indices[i++] = c;
			indices[i++] = b;
			// triangle two
			indices[i++] = c;
			indices[i++] = a;
			indices[i++] = d;
		}
	}
	BufferData();

	heightMapSize.x = vertexScale.x * (iWidth - 1);
	heightMapSize.y = vertexScale.y * 255.0f;
	heightMapSize.z = vertexScale.z * (iHeight - 1);
}