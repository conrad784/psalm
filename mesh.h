/*!
*	@file	mesh.h
*	@brief	Data structures for representing a mesh
*/

#ifndef __MESH_H__
#define __MESH_H__

#include <vector>
#include <string>

#include "vertex.h"
#include "edge.h"
#include "face.h"

#include "t_edge_hash.h"

/*!
*	@class mesh
*	@brief Represents a mesh
*/

class mesh
{
	public:
		mesh();
		~mesh();

		bool load(const std::string& filename, const short type = TYPE_EXT);
		bool save(const std::string& filename, const short type = TYPE_EXT);

		void subdivide_loop();
		void subdivide_doo_sabin();
		void subdivide_catmull_clark();

		void draw();
		void destroy();

		mesh& operator=(const mesh& M);
		mesh& replace_with(mesh& M);

	private:
		t_edge_hash	edge_table;

		std::vector<vertex*>	V;
		std::vector<face*>	F;

		vertex* get_vertex(size_t id);
		edge* get_edge(size_t e);

		void add_face(const std::vector<size_t>& vertices);
		vertex* add_vertex(double x, double y, double z);

		const vertex* find_remaining_vertex(const edge* e, const face* f);
		const vertex* find_face_vertex(const face* f, const vertex* v);

		std::vector<const face*> sort_faces(const vertex* v) const;

		bool load_ply(const char* filename);
		bool load_obj(const char* filename);
		bool load_off(const char* filename);

		bool save_ply(const char* filename);
		bool save_obj(const char* filename);
		bool save_off(const char* filename);

		static const short TYPE_PLY; //< Constant for reading/writing PLY files
		static const short TYPE_OBJ; //< Constant for reading/writing OBJ files
		static const short TYPE_OFF; //< Constant for reading/writing OFF files
		static const short TYPE_EXT; //< Constant for reading/writing files by their extension
};

#endif
