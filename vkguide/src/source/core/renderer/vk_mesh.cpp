#include <pch.hpp>
#include "vk_mesh.hpp"

VertexInputDescription Vertex::get_vertex_description()
{
  VertexInputDescription vid;

  VkVertexInputAttributeDescription;
  VkVertexInputBindingDescription;
  VkPipelineVertexInputStateCreateFlags;

  VkVertexInputBindingDescription vertBinding{
    .binding = 0,
    .stride = sizeof Vertex,
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
  };

  vid.bindings = { vertBinding };

  VkVertexInputAttributeDescription posAttrib{
    .location = 0,
    .binding = 0,
    .format = VK_FORMAT_R32G32B32_SFLOAT,
    .offset = offsetof(Vertex, position)
  };

  VkVertexInputAttributeDescription normalAttrib{
    .location = 1,
    .binding = 0,
    .format = VK_FORMAT_R32G32B32_SFLOAT,
    .offset = offsetof(Vertex, normal)
  };

  VkVertexInputAttributeDescription colorAttrib{
    .location = 2,
    .binding = 0,
    .format = VK_FORMAT_R32G32B32_SFLOAT,
    .offset = offsetof(Vertex, color)
  };

  vid.attributes = {
    posAttrib,
    normalAttrib,
    colorAttrib
  };

  return vid;
}

// TODO: Fix loading non-triangulated meshes
// TODO: Fix loading materials
// TODO: Hook up to logging once implemented
Mesh load_from_obj(const std::string& filepath, const std::string& mtlDir)
{
  Mesh m{};

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  
  std::string warn, err;

	tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), mtlDir.c_str());

  if (!warn.empty())
  {
    std::cout << "WARN: " << warn << '\n';
  }

  if (!err.empty())
  {
    std::cerr << "ERROR: " << warn << '\n';
    return m;
  }

  // Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++)
  {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
    {
      //hardcode loading to triangles
			int fv = 3;

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++)
      {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

        //vertex position
				tinyobj::real_t vx = attrib.vertices[3ull * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3ull * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3ull * idx.vertex_index + 2];

        //vertex normal
        tinyobj::real_t nx = attrib.normals[3ull * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3ull * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3ull * idx.normal_index + 2];

        //copy it into our vertex
				Vertex new_vert;
				new_vert.position.x = vx;
				new_vert.position.y = vy;
				new_vert.position.z = vz;

				new_vert.normal.x = nx;
				new_vert.normal.y = ny;
        new_vert.normal.z = nz;

        //we are setting the vertex color as the vertex normal. This is just for display purposes
        new_vert.color = new_vert.normal;

				m.vertices.push_back(new_vert);
			}

			index_offset += fv;
		}
	}

  return m;
}
