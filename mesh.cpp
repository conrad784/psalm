/*!
*	@file	mesh.cpp
*	@brief	Functions for representing a mesh
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <limits>

#include <ctime>
#include <cmath>
#include <cassert>
#include <cerrno>
#include <cstring>

#include "mesh.h"

namespace psalm
{

// Initialization of some static member variables

const short mesh::TYPE_EXT		= 0;
const short mesh::TYPE_PLY		= 1;
const short mesh::TYPE_OBJ		= 2;
const short mesh::TYPE_OFF		= 3;

const short mesh::STATUS_OK		= 0;
const short mesh::STATUS_ERROR		= 1;
const short mesh::STATUS_UNDEFINED	= 2;

const short mesh::ALG_CATMULL_CLARK	= 0;
const short mesh::ALG_DOO_SABIN		= 1;
const short mesh::ALG_LOOP		= 2;

const short mesh::W_DEFAULT		= 0;
const short mesh::W_CATMULL_CLARK	= 1;
const short mesh::W_DOO_SABIN		= 2;

/*!
*	Default constructor.
*/

mesh::mesh()
{
}

/*!
*	Destructor for mesh. Calls destroy() function (if the user did not
*	already delete data).
*/

mesh::~mesh()
{
	destroy();
}

/*!
*	Tries to load data (presumably mesh data) that from an input source the
*	user specified. The type of the data is determined by the following
*	process:
*
*	1)	If the user did not specify a type:
*
*		1.1)	If the user specified a filename, the function tries to
*		identify the file by its extension.
*
*			1.1.1)	If this identification fails, the function
*			tries to load a PLY file.
*
*		1.2)	If the user did not specify a filename, the function
*		tries to load PLY data from standard input.
*
*	2)	If the user specified a type:
*
*		2.1)	If the user specified a filename, the function tries to
*		load the file with the appropriate type set, regardless of its
*		extension.
*
*		2.2)	If the user did not specify a filename, the function
*		tries to load data with the specified type from standard input.
*
*	@param filename Filename of data file. An empty filename signals that
*	the function tries to read data from standard input.
*
*	@param type Type of mesh data to load. By default, the function tries
*	to guess the data type using filename extensions (if the user specified
*	a filename).
*
*	@return	true if the mesh could be loaded, else false
*/

bool mesh::load(const std::string& filename, short type)
{
	short result = STATUS_UNDEFINED;

	std::ifstream in;
	if(filename.length() > 0)
	{
		errno = 0;
		in.open(filename.c_str());
		if(errno)
		{
			std::string error = strerror(errno);
			std::cerr	<< "psalm: Could not load input file \""
					<< filename << "\": "
					<< error << "\n";

			return(false);
		}
	}

	this->destroy();

	// Filename given, data type identification by extension
	if(filename.length() >= 4 && type == TYPE_EXT)
	{
		std::string extension = filename.substr(filename.length()-4);
		std::transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int)) tolower);

		if(extension == ".ply")
			result = (load_ply(in) ? STATUS_OK : STATUS_ERROR);
		else if(extension == ".obj")
			result = (load_obj(in) ? STATUS_OK : STATUS_ERROR);
		else if(extension == ".off")
			result = (load_off(in) ? STATUS_OK : STATUS_ERROR);

		// Unknown extension, so we fall back to PLY files (see below)
	}

	// Data type specified
	else if(type != TYPE_EXT)
	{
		// Check whether file name has been specified. If no file name
		// has been specified, use standard input to read data.

		std::istream& input_stream = ((filename.length() > 0) ? in : std::cin);
		switch(type)
		{
			case TYPE_PLY:
				result = (load_ply(input_stream) ? STATUS_OK : STATUS_ERROR);
				break;

			case TYPE_OBJ:
				result = (load_obj(input_stream) ? STATUS_OK : STATUS_ERROR);
				break;

			case TYPE_OFF:
				result = (load_off(input_stream) ? STATUS_OK : STATUS_ERROR);
				break;
		}
	}

	// Last resort: If a nonempty file name has been specified, try to
	// parse a PLY file. Else, try to read a PLY file from standard input.
	if(result == STATUS_UNDEFINED)
	{
		if(filename.length() > 0)
			result = (load_ply(in) ? STATUS_OK : STATUS_ERROR);
		else
			result = (load_ply(std::cin) ? STATUS_OK : STATUS_ERROR);
	}

	in.close();
	return(result);
}

/*!
*	Tries to save the current mesh data to a user-specified output (a file
*	or an output stream).  The format of the mesh data is determined by the
*	following process:
*
*	1)	If the user did not specify a type:
*
*		1.1)	If the user specified a filename, the function tries to
*		identify the file by its extension.
*
*			1.1.1)	If this identification fails, the function
*			tries to save a PLY file.
*
*		1.2)	If the user did not specify a filename, the function
*		tries to save PLY data to standard output.
*
*	2)	If the user specified a type:
*
*		2.1)	If the user specified a filename, the function tries to
*		save the file with the appropriate type set, regardless of its
*		extension.
*
*		2.2)	If the user did not specify a filename, the function
*		tries to save data with the specified type to standard output.
*
*	@param filename Filename for storing the current mesh. An empty
*	filename signals that standard output is to be used.
*
*	@param type Format in which to save the mesh data. By default, the
*	function tries to guess the data type using filename extensions (if the
*	user specified a filename).
*
*	@warning The data file will be overwritten if it exists. The user will
*	not be notified of this.
*
*	@return	true if the mesh could be stored, else false.
*/

bool mesh::save(const std::string& filename, short type)
{
	short result = STATUS_UNDEFINED;

	std::ofstream out;
	if(filename.length() > 0)
	{
		errno = 0;
		out.open(filename.c_str());
		if(errno)
		{
			std::string error = strerror(errno);
			std::cerr	<< "psalm: Could not save to file \""
					<< filename << "\": "
					<< error << "\n";

			return(false);
		}
	}

	// Filename given, data type identification by extension
	if(filename.length() >= 4 && type == TYPE_EXT)
	{
		std::string extension = filename.substr(filename.length()-4);
		std::transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int)) tolower);

		if(extension == ".ply")
			result = (save_ply(out) ? STATUS_OK : STATUS_ERROR);
		else if(extension == ".obj")
			result = (save_obj(out) ? STATUS_OK : STATUS_ERROR);
		else if(extension == ".off")
			result = (save_off(out) ? STATUS_OK : STATUS_ERROR);

		// Unknown extension, so we fall back to PLY files (see below)
	}

	// Data type specified
	else if(type != TYPE_EXT)
	{
		// Check whether file name has been specified. If no file name
		// has been specified, use standard output to write data.

		std::ostream& output_stream = ((filename.length() > 0) ? out : std::cout);

		switch(type)
		{
			case TYPE_PLY:
				result = (save_ply(output_stream) ? STATUS_OK : STATUS_ERROR);
				break;

			case TYPE_OBJ:
				result = (save_obj(output_stream) ? STATUS_OK : STATUS_ERROR);
				break;

			case TYPE_OFF:
				result = (save_off(output_stream) ? STATUS_OK : STATUS_ERROR);
				break;
		}
	}

	// Last resort: If a nonempty filename has been specified, try to
	// write a PLY file. Else, try to write a PLY file to standard output.
	if(result == STATUS_UNDEFINED)
	{
		if(filename.length() > 0)
			result = (save_ply(out) ? STATUS_OK : STATUS_ERROR);
		else
			result = (save_ply(std::cout) ? STATUS_OK : STATUS_ERROR);
	}

	out.close();
	return(result);
}

/*!
*	Tries to load mesh data in PLY format from an input stream.
*
*	@param	in Input stream (file, standard input)
*	@return	true if the mesh could be loaded, else false
*/

bool mesh::load_ply(std::istream& in)
{
	if(!in.good())
		return(false);

	std::string data;

	// Read the headers: Only ASCII format is accepted, but the version is
	// ignored

	std::getline(in, data);
	if(data != "ply")
	{
		std::cerr << "psalm: I am missing a \"ply\" header for the input data.\n";
		return(false);
	}

	std::getline(in, data);
	if(data.find("format ascii") == std::string::npos)
	{
		std::cerr << "psalm: Expected \"format ascii\", got \"" << data << "\" instead.\n";
		return(false);
	}

	/*
		Parsing further element properties is quick and dirty: It is
		assumed that face data is declared _after_ the vertex data.
		Properties are assumed to come in the natural order, i.e.:

			x
			y
			z

		for vertex data.
	*/

	size_t num_vertices	= 0;
	size_t num_faces	= 0;

	const short MODE_PARSE_HEADER			= 0;
	const short MODE_PARSE_VERTEX_PROPERTIES	= 1;
	const short MODE_PARSE_FACE_PROPERTIES		= 2;

	short mode = MODE_PARSE_HEADER;
	while(!in.eof())
	{
		getline(in, data);

		/*
			Lines contaning "comment" or "obj_info" are skipped.
			Not sure whether obj_info is allowed to appear at all.
		*/
		if(	data.find("comment")  != std::string::npos ||
			data.find("obj_info") != std::string::npos)
			continue;
		else if(data.find("end_header") != std::string::npos)
			break;

		switch(mode)
		{
			case MODE_PARSE_VERTEX_PROPERTIES:

				if(data.find("property") != std::string::npos)
				{
					/*
						Ignore. Some special handlings
						for more properties could be
						added here.
					*/

					continue;
				}
				else if(data.find("element face") != std::string::npos)
				{
					mode = MODE_PARSE_FACE_PROPERTIES;

					std::string dummy; // not necessary, but more readable
					std::istringstream converter(data);
					converter >> dummy >> dummy >> num_faces;

					if(num_faces == 0)
					{
						std::cerr	<< "psalm: Can't parse number of faces from \""
								<< data
								<< "\".\n";
						return(false);
					}

					mode = MODE_PARSE_FACE_PROPERTIES;
				}
				else
				{
					std::cerr << "psalm: Expected \"property\", but got \"" << data << "\" instead.\n";
					return(false);
				}

				break;

			case MODE_PARSE_FACE_PROPERTIES:

				if(data.find("property list") == std::string::npos)
				{
					std::cerr << "Warning: Got \"" << data << "\". "
					<< "This property is unknown and might lead "
					<< "to problems when parsing the file.\n";
				}

				break;

			// Expect "element vertex" line
			case MODE_PARSE_HEADER:

				if(data.find("element vertex") != std::string::npos)
				{
					mode = MODE_PARSE_VERTEX_PROPERTIES;

					std::string dummy; // not necessary, but more readable
					std::istringstream converter(data);
					converter >> dummy >> dummy >> num_vertices;

					if(num_vertices == 0)
					{
						std::cerr	<< "psalm: Can't parse number of vertices from \""
								<< data
								<< "\".\n";

						return(false);
					}

					mode = MODE_PARSE_VERTEX_PROPERTIES;
				}
				else
				{
					std::cerr	<< "psalm: Got \""
							<< data
							<< "\", but expected \"element vertex\" "
							<< "or \"element face\" instead. I cannot continue.\n";
					return(false);
				}

				break;

			default:
				break;
		}
	}

	size_t line	= 0;
	size_t k	= 0; // number of vertices for face

	double x, y, z;
	while(!in.eof())
	{
		if(line < num_vertices)
		{
			in >> x >> y >> z;
			add_vertex(x, y, z);
		}
		else
		{
			k = 0;
			in >> k;
			if(k == 0)
				break;

			// Store vertices of face in proper order and add a new
			// face.

			std::vector<vertex*> vertices;
			size_t v = 0;
			for(size_t i = 0; i < k; i++)
			{
				in >> v;
				vertices.push_back(get_vertex(v));
			}

			add_face(vertices);
		}

		line++;
	}

	return(true);
}

/*!
*	Saves the currently loaded mesh in PLY format.
*
*	@param	out Stream for data output
*	@return	true if the mesh could be stored, else false.
*/

bool mesh::save_ply(std::ostream& out)
{
	if(!out.good())
		return(false);

	// header information
	out	<< "ply\n"
		<< "format ascii 1.0\n"
		<< "element vertex " << V.size() << "\n"
		<< "property float x\n"
		<< "property float y\n"
		<< "property float z\n"
		<< "element face " << F.size() << "\n"
		<< "property list uchar int vertex_indices\n"
		<< "end_header\n";

	// write vertex list (separated by spaces)
	for(size_t i = 0; i < V.size(); i++)
	{
		out << std::fixed << std::setprecision(8) << V[i]->get_position()[0] << " "
						<< V[i]->get_position()[1] << " "
						<< V[i]->get_position()[2] << "\n";
	}

	// write face list (separated by spaces)
	for(size_t i = 0; i < F.size(); i++)
	{
		out << F[i]->num_vertices() << " ";
		for(size_t j = 0; j < F[i]->num_vertices(); j++)
		{
			out << F[i]->get_vertex(j)->get_id();
			if(j < F[i]->num_vertices()-1)
				out << " ";
		}

		out << "\n";
	}

	return(true);
}

/*!
*	Loads a mesh in Wavefront OBJ format from an input stream. Almost all
*	possible information from the input stream will be ignored gracefully
*	because the program is only interested in the raw geometrical data.
*
*	@param	in Input stream (file, standard input)
*	@return	true if the mesh could be loaded, else false
*/

bool mesh::load_obj(std::istream &in)
{
	if(!in.good())
		return(false);

	std::string line;
	std::string keyword;
	std::istringstream converter;

	// These are specify the only keywords of the .OBJ file that the parse
	// is going to understand

	const std::string OBJ_KEY_VERTEX	= "v";
	const std::string OBJ_KEY_FACE		= "f";

	while(!std::getline(in, line).eof())
	{
		converter.str(line);
		converter >> keyword;

		if(keyword == OBJ_KEY_VERTEX)
		{
			double x, y, z;
			converter >> x >> y >> z;

			if(converter.fail())
			{
				std::cerr	<< "psalm: I tried to parse vertex coordinates from line \""
						<< line
						<<" \" and failed.\n";
				return(false);
			}

			add_vertex(x,y,z);
		}
		else if(keyword == OBJ_KEY_FACE)
		{
			// Check whether it is a triplet data string
			if(line.find_first_of('/') != std::string::npos)
			{
				// FIXME: NYI
			}
			else
			{
				std::vector<vertex*> vertices;

				long index = 0;
				while(!converter.eof())
				{
					index = 0;
					converter >> index;

					if(index == 0)
					{
						std::cerr	<< "psalm: I cannot parse face data from line \""
								<< line
								<< "\".\n";
						return(false);
					}

					// Handle backwards references...
					else if(index < 0)
					{
						// ...and check the range
						if((V.size()+index) >= 0)
							vertices.push_back(get_vertex(V.size()+index));
						else
						{
							std::cerr	<< "psalm: Invalid backwards vertex reference "
									<< "in line \""
									<< line
									<< "\".\n";
							return(false);
						}
					}
					else
						vertices.push_back(get_vertex(index-1)); // Real men 0-index their variables.
				}

				add_face(vertices);
			}
		}

		keyword.clear();
		line.clear();
		converter.clear();
	}

	return(true);
}

/*!
*	Saves the currently loaded mesh in Wavefront OBJ format. Only raw
*	geometrical data will be output.
*
*	@param	out Stream for data output
*	@return	true if the mesh could be stored, else false.
*/

bool mesh::save_obj(std::ostream& out)
{
	if(!out.good())
		return(false);

	for(std::vector<vertex*>::const_iterator it = V.begin(); it != V.end(); it++)
	{
		v3ctor position = (*it)->get_position();
		out << "v "	<< position[0] << " "
				<< position[1] << " "
				<< position[2] << "\n";
	}

	for(std::vector<face*>::const_iterator it = F.begin(); it != F.end(); it++)
	{
		out << "f ";
		for(size_t i = 0; i < (*it)->num_vertices(); i++)
		{
			out << ((*it)->get_vertex(i)->get_id()+1); // OBJ is 1-indexed, not 0-indexed
			if(i < (*it)->num_vertices()-1)
				out << " ";
		}
		out << "\n";
	}

	return(true);
}

/*!
*	Loads a mesh in ASCII Geomview format from an input stream.
*
*	@param	in Input stream (file, standard input)
*	@return	true if the mesh could be loaded, else false
*/

bool mesh::load_off(std::istream& in)
{
	if(!in.good())
		return(false);

	std::string line;
	std::istringstream converter;

	/*
		Read "header", i.e.,

			OFF
			num_vertices num_faces num_edges

		where num_edges is ignored.
	*/

	std::getline(in, line);
	if(line != "OFF")
	{
		std::cerr << "psalm: I am missing a \"OFF\" header for the input data.\n";
		return(false);
	}

	size_t num_vertices, num_faces, num_edges;
	size_t cur_line_num = 0; // count line numbers (after header)

	std::getline(in, line);
	converter.str(line);
	converter >> num_vertices >> num_faces >> num_edges;

	if(converter.fail())
	{
		std::cerr << "psalm: I cannot parse vertex, face, and edge numbers from \"" << line << "\"\n";
		return(false);
	}

	converter.clear();
	line.clear();

	// These are specify the only keywords of the .OBJ file that the parse
	// is going to understand

	while(!std::getline(in, line).eof())
	{
		converter.str(line);

		if(cur_line_num < num_vertices)
		{
			double x, y, z;
			converter >> x >> y >> z;

			if(converter.fail())
			{
				std::cerr	<< "psalm: I tried to parse vertex coordinates from line \""
						<< line
						<<" \" and failed.\n";
				return(false);
			}

			add_vertex(x,y,z);
		}
		else if((cur_line_num-num_vertices) < num_faces)
		{
			size_t k	= 0;
			size_t index	= 0;

			converter >> k;

			std::vector<vertex*> vertices;
			for(size_t i = 0; i < k; i++)
			{
				converter >> index;
				if(converter.fail())
				{
					std::cerr	<< "psalm: Tried to parse face data in line \""
							<< line
							<< "\", but failed.\n";
					return(false);
				}

				if(index >= V.size())
				{
					std::cerr	<< "psalm: Index " << index << "in line \""
							<< line
						<< "\" is out of bounds.\n";
					return(false);
				}

				vertices.push_back(get_vertex(index));
			}

			add_face(vertices);
		}
		else
		{
			std::cerr << "psalm: Got an unexpected data line \"" << line << "\".\n";
			return(false);
		}

		cur_line_num++;

		converter.clear();
		line.clear();
	}

	return(true);
}

/*!
*	Saves the currently loaded mesh in ASCII Geomview object file format
*	(OFF).
*
*	@param	out Stream for data output
*	@return	true if the mesh could be stored, else false.
*/

bool mesh::save_off(std::ostream& out)
{
	if(!out.good())
		return(false);

	out	<< "OFF\n"
		<< V.size() << " " << F.size() << " " << "0\n"; // For programs that actually interpret edge data,
								// the last parameter should be changed

	for(std::vector<vertex*>::const_iterator it = V.begin(); it != V.end(); it++)
	{
		v3ctor position = (*it)->get_position();
		out	<< position[0] << " "
			<< position[1] << " "
			<< position[2] << "\n";
	}

	for(std::vector<face*>::const_iterator it = F.begin(); it != F.end(); it++)
	{
		out	<< (*it)->num_vertices()
			<< " ";

		for(size_t i = 0; i < (*it)->num_vertices(); i++)
		{
			out << (*it)->get_vertex(i)->get_id();
			if(i < (*it)->num_vertices()-1)
				out << " ";
		}
		out << "\n";
	}

	return(true);
}

/*!
*	Destroys the mesh and all related data structures and frees up used
*	memory.
*/

void mesh::destroy()
{
	for(std::vector<vertex*>::iterator it = V.begin(); it != V.end(); it++)
	{
		if(*it != NULL)
			delete(*it);
	}

	for(std::vector<edge*>::iterator it = E.begin(); it != E.end(); it++)
	{
		if(*it != NULL)
			delete(*it);
	}

	for(std::vector<face*>::iterator it = F.begin(); it != F.end(); it++)
	{
		if(*it != NULL)
			delete(*it);
	}

	V.clear();
	E.clear();
	F.clear();

	E_M.clear();
}

/*!
*	Replaces the current mesh with another one. The other mesh will
*	be deleted/cleared by this operation.
*
*	@param	M Mesh to replace current mesh with
*/

void mesh::replace_with(mesh& M)
{
	*this = M;

	// Clear old mesh
	M.V.clear();
	M.F.clear();
	M.E.clear();
	M.E_M.clear();
}

/*!
*	Assigns another mesh to the current mesh.
*
*	@param M Mesh to assign to the current mesh
*	@return Reference to replaced mesh
*/

mesh& mesh::operator=(const mesh& M)
{
	this->destroy();

	this->V		= M.V;
	this->F		= M.F;
	this->E		= M.E;
	this->E_M	= M.E_M;

	return(*this);
}

/*!
*	Given a vector of pointers to vertices, where the vertices are assumed
*	to be in counterclockwise order, construct a face and add it to the
*	mesh.
*
*	@param vertices Vector of vertices for face. The vertices are connected
*	in the order they appear in the vector. A last edge from the last
*	vertex to the first vertex is added.
*
*	@warning The orientation of the vertices around the face is _not_
*	checked, but left as a task for the calling function.
*/

void mesh::add_face(std::vector<vertex*> vertices)
{
	vertex* u = NULL;
	vertex* v = NULL;

	if(vertices.size() == 0)
		return;

	face* f = new face;

	std::vector<vertex*>::iterator it = vertices.begin();
	u = *it;

	for(it = vertices.begin(); it != vertices.end(); it++)
	{
		// Handle last vertex; should be the edge v--u
		if((it+1) == vertices.end())
		{
			u = *it;
			v = *vertices.begin();
		}
		// Normal case
		else
			v = *(it+1);

		// Add vertex to face; only the first vertex of the edge needs
		// to be considered here
		f->add_vertex(u);

		// Add it to list of edges for face
		directed_edge edge = add_edge(u, v);
		f->add_edge(edge);

		/*
			GIANT FIXME: We are assuming that the edges are ordered
			properly. Hence, an edge is supposed to appear only
			_once_ in a fixed direction. If this is not the case,
			the lookup below will _fail_ or an already stored face
			might be overwritten!
		*/

		// Edge already known; update second adjacent face
		if(edge.inverted)
		{
			edge.e->set_g(f);
			//V[u]->add_edge(edge.e);
			u->add_face(f);
		}

		// (Possibly) new edge; update first adjacent face and adjacent
		// vertices
		else
		{
			// FIXME: This is ugly...and probably wrong?
			if(edge.new_edge)
			{
				edge.e->set_f(f);
				u->add_edge(edge.e);
				// TODO: Check whether it's ok to do this...or if it
				// can be removed and done for the edge.inverted ==
				// true case
				v->add_edge(edge.e);
			}
			else
				edge.e->set_g(f);

			u->add_face(f);
		}

		// Set next start vertex; the orientation should be correct
		// here
		u = v;
	}

	F.push_back(f);
}

/*!
*	Adds a triangular face to the mesh. This function allows the caller to
*	specify 3 vertices that will form the new triangle. Thus, specifying a
*	vector of pointers is not necessary.
*
*	@param v1 Pointer to 1st vertex of new face
*	@param v2 Pointer to 2nd vertex of new face
*	@param v3 Pointer to 3rd vertex of new face
*
*	@warning The vertex pointers are not checked for consistency.
*/

inline void mesh::add_face(vertex* v1, vertex* v2, vertex* v3)
{
	std::vector<vertex*> vertices;

	vertices.push_back(v1);
	vertices.push_back(v2);
	vertices.push_back(v3);

	add_face(vertices);
}

/*!
*	Adds a quadrangular face to the mesh. This function allows the caller
*	to specify 4 vertices that will form the new quadrangle. Thus,
*	specifying a vector of pointers is not necessary.
*
*	@param v1 Pointer to 1st vertex of new face
*	@param v2 Pointer to 2nd vertex of new face
*	@param v3 Pointer to 3rd vertex of new face
*	@param v4 Pointer to 4th vertex of new face
*
*	@warning The vertex pointers are not checked for consistency and
*	planarity.
*/

inline void mesh::add_face(vertex* v1, vertex* v2, vertex* v3, vertex* v4)
{
	std::vector<vertex*> vertices;

	vertices.push_back(v1);
	vertices.push_back(v2);
	vertices.push_back(v3);
	vertices.push_back(v4);

	add_face(vertices);
}

/*!
*	Tries to add an edge to the current mesh. The edge is described by
*	pointers to its start and end vertices.
*
*	@param u Pointer to start vertex
*	@param v Pointer to end vertex
*
*	@return A directed edge, i.e., an edge with a certain start and end
*	vertex and a flag whether the edge has been inverted or not. If the
*	edge has been inverted, start and end vertex change their meaning. This
*	is required because from the point of the medge, edge (u,v) and edge
*	(v,u) are _the same_. In order to store only an edge only one time, the
*	mesh checks whether the edge already exists.
*/

directed_edge mesh::add_edge(const vertex* u, const vertex* v)
{
	directed_edge result;

	/*
		The edge is assigned an ID string of the form "k1-k2", where k1
		is the lower of the two indices.

		Previously, the Cantor pairing function had been used, but this
		yielded integer overflows with normal 32bit integers. Hence,
		the std::string attempt is better suited to this task, although
		it requires a conversion step.
	*/

	size_t k1, k2;
	if(u->get_id() < v->get_id())
	{
		k1 = u->get_id();
		k2 = v->get_id();
	}
	else
	{
		k1 = v->get_id();
		k2 = u->get_id();
	}

	std::ostringstream converter;
	converter << k1 << "-" << k2;
	std::string k = converter.str();

	// Check whether edge exists
	std::tr1::unordered_map<std::string, edge*>::iterator it;
	if((it = E_M.find(k)) == E_M.end())
	{
		// Edge not found, create an edge from the _original_ edge and
		// add it to the map
		edge* new_edge = new edge(u, v);
		E.push_back(new_edge);
		E_M[k] = new_edge;

		result.e = new_edge;
		result.inverted = false;
		result.new_edge = true;
	}
	else
	{
		// Edge has been found, check whether the proper direction has
		// been stored.
		if(it->second->get_u()->get_id() != u->get_id())
			result.inverted = true;
		else
			result.inverted = false;

		result.new_edge = false;
		result.e = it->second;
	}

	return(result);
}

/*!
*	Given a vertex ID, return the appropriate vertex. This function is
*	meant to serve as an interface for any vertex queries, regardless of
*	what storage container the mesh uses.
*
*	@param	Vertex ID, which is supposed to be set on allocating/creating a
*	new vertex.
*
*	@return If the ID is correct, a pointer to the corresponding vertex is
*	returned. Else, a NULL pointer will be rereturned.
*/

vertex* mesh::get_vertex(size_t id)
{
	if(id >= V.size())
		return(NULL);
	else
		return(V[id]);
}

/*!
*	Adds a vertex to the mesh. The vertex ID is assigned automatically.
*
*	@param x x position of vertex
*	@param y y position of vertex
*	@param z z position of vertex
*
*	@warning The vertices are not checked for duplicates because this
*	function is assumed to be called from internal methods only.
*
*	@return Pointer to new vertex. The pointer remains valid during the
*	lifecycle of the mesh.
*/

vertex* mesh::add_vertex(double x, double y, double z)
{
	vertex* v = new vertex(x,y,z, V.size());
	V.push_back(v);

	return(v);
}

/*!
*	Adds a vertex to the mesh. The vertex ID is assigned automatically.
*
*	@param pos Position of the new vertex
*
*	@warning The vertices are not checked for duplicates because this
*	function is assumed to be called from internal methods only.
*
*	@return Pointer to new vertex. The pointer remains valid during the
*	lifecycle of the mesh.
*/

vertex* mesh::add_vertex(const v3ctor& pos)
{
	return(add_vertex(pos[0], pos[1], pos[2]));
}

/*!
*	Performs several pruning operations on the current mesh:
*
*		- Removal of faces with n sides
*
*	@param ignore_faces	Contains a set of numbers. If a face with n
*				sides is found and n matches one of these
*				numbers, the face will be removed from the
*				mesh.
*/

void mesh::prune(const std::set<size_t>& ignore_faces)
{
	for(std::vector<face*>::iterator it = F.begin(); it != F.end(); it++)
	{
		if(ignore_faces.find((*it)->num_edges()) != ignore_faces.end())
			it = F.erase(it);
	}
}

/*!
*	Applies a subdivision algorithm to the current mesh.
*
*	@param algorithm	Defines which algorithm to use (Catmull-Clark
*				if the user did not specify otherwise).
*	@param steps		Number of subdivision steps (1 if the user did
*				not specify otherwise).
*	@param weights		Defines which weight distribution function is
*				to be used for the given subdivision scheme. By
*				default, the standard weights for any algorithm
*				are used.
*	@param extra_weights	Pointer to an associative array that contains
*				the weights for a face with a given number of
*				vertices. NULL by default.
*/

void mesh::subdivide(	short algorithm,
			size_t steps,
			short weights,
			const weights_map* extra_weights)
{
	// Choose algorithm (if this is _not_ done via pointers, the for-loop
	// would have to be duplicated or the algorithm check would have to be
	// made for each iteration.
	void (mesh::*subdivision_algorithm)(short, const weights_map*) = NULL;
	switch(algorithm)
	{
		case ALG_CATMULL_CLARK:
			subdivision_algorithm = &mesh::subdivide_catmull_clark;
			break;
		case ALG_DOO_SABIN:
			subdivision_algorithm = &mesh::subdivide_doo_sabin;
			break;
		case ALG_LOOP:
			subdivision_algorithm = &mesh::subdivide_loop;
			break;
		default:
			break;
	};

	for(size_t i = 0; i < steps; i++)
		(this->*subdivision_algorithm)(weights, extra_weights);
}

/*!
*	Performs one step of Loop subdivision on the current mesh, thereby
*	replacing it with the refined mesh.
*
*	@param weights		Parameter is ignored for now
*	@param extra_weights	Parameter is ignored for now
*/

void mesh::subdivide_loop(short weights, const weights_map* extra_weights)
{
	mesh M;

	// Construct vertex points
	for(size_t i = 0; i < V.size(); i++)
	{
		// Find neighbours

		size_t n = V[i]->valency();
		v3ctor vertex_point;

		// TODO: Could also be done using iterators.
		for(size_t j = 0; j < n; j++)
		{
			const edge* e = V[i]->get_edge(j);

			/*
				i is the index of the current vertex; if the
				start vertex of the edge is _not_ the current
				vertex, it must be the neighbouring vertex.
			*/

			const vertex* neighbour = (e->get_u()->get_id() != V[i]->get_id()? e->get_u() : e->get_v());
			vertex_point += neighbour->get_position();
		}

		double s = 0.0;
		if(n > 3)
			s = (1.0/n*(0.625-pow(0.375+0.25*cos(2*M_PI/n), 2)));
		else
			s = 0.1875;

		vertex_point *= s;
		vertex_point += V[i]->get_position()*(1.0-n*s);

		V[i]->vertex_point = M.add_vertex(vertex_point);
	}

	// Create edge points
	for(std::vector<edge*>::iterator it = E.begin(); it != E.end(); it++)
	{
		v3ctor edge_point;
		edge* e = *it;

		// Find remaining vertices of the adjacent faces of the edge
		const vertex* v1 = find_remaining_vertex(e, e->get_f());
		const vertex* v2 = find_remaining_vertex(e, e->get_g());

		// Boundary edge: Use midpoint subdivision
		if(v1 == NULL || v2 == NULL)
		{
			edge_point = (	(e->get_u()->get_position()+
					 e->get_v()->get_position())*0.5);
		}

		// Normal edge
		else
		{
			edge_point =	(e->get_u()->get_position()+e->get_v()->get_position())*0.375+
					(v1->get_position()+v2->get_position())*0.125;
		}

		e->edge_point = M.add_vertex(edge_point);
	}

	// Create topology for new mesh
	for(size_t i = 0; i < F.size(); i++)
	{
		// ...go through all vertices of the face
		for(size_t j = 0; j < F[i]->num_vertices(); j++)
		{
			/*
				F[i].V[j] is the current vertex of a face. We
				now need to find the _two_ adjacent edges for
				the face. This yields one new triangle.
			*/

			size_t n = F[i]->num_edges(); // number of edges in face
			bool assigned_first_edge = false;

			directed_edge d_e1; // first adjacent edge (for vertex & face)
			directed_edge d_e2; // second adjacent edge (for vertex & face)

			// brute-force search for the two edges
			//
			// TODO: Optimize!
			for(size_t k = 0; k < n; k++)
			{
				directed_edge d_edge = F[i]->get_edge(k);
				if(	d_edge.e->get_u()->get_id() == F[i]->get_vertex(j)->get_id() ||
					d_edge.e->get_v()->get_id() == F[i]->get_vertex(j)->get_id())
				{
					if(!assigned_first_edge)
					{
						d_e1 = d_edge;
						assigned_first_edge = true;
					}
					else
					{
						d_e2 = d_edge;
						break;
					}
				}
			}

			vertex* v1 = F[i]->get_vertex(j)->vertex_point;
			vertex* v2 = d_e1.e->edge_point;
			vertex* v3 = d_e2.e->edge_point;

			/*
				 Create vertices for _new_ face. It is
				 important to determine the proper order of the
				 edges here. The new edges should run "along"
				 the old ones.

				 Hence, it is checked whether the current
				 vertex is the _start_ vertex of the first
				 edge. This is the case if _either_ the edge is
				 not inverted and the current vertex is equal
				 to the vertex u (start vertex) of the edge
				 _or_ the edge is inverted and the current
				 vertex is equal to the vertex v (end vertex)
				 of the edge.

				 If the current vertex is the start vertex of
				 the first edge, connecting the new points in
				 order v1, v2, v3 will create a face that is
				 oriented counterclockwise. Else, order v1, v3,
				 v2 will have to be used.

				 v1 can remain fixed in all cases because of
				 the symmetry.
			*/

			if((d_e1.e->get_u()->get_id() == F[i]->get_vertex(j)->get_id() && d_e1.inverted == false) ||
			   (d_e1.e->get_v()->get_id() == F[i]->get_vertex(j)->get_id() && d_e1.inverted))
				M.add_face(v1, v2, v3);

			// Swap order
			else
				M.add_face(v1, v3, v2);
		}

		// Create face from all three edge points of the face; since
		// the edges are stored in the proper order when adding the
		// face, the order in which the edge points are set will be
		// correct.

		if(F[i]->num_edges() != 3)
		{
			std::cerr << "psalm: Input mesh contains non-triangular face. Loop's subdivision scheme is not applicable.\n";
			return;
		}

		M.add_face(	F[i]->get_edge(0).e->edge_point,
				F[i]->get_edge(1).e->edge_point,
				F[i]->get_edge(2).e->edge_point);
	}

	this->replace_with(M);
}

/*!
*	Performs one step of Doo-Sabin subdivision on the current mesh.
*
*	@param weights		Allows the user to choose different weights for the
*				algorithm. The values W_CATMULL_CLARK and
*				W_DOO_SABIN are possible.
*	@param extra_weights	Allows the user to override certain weights for
*				the algorithm.
*/

void mesh::subdivide_doo_sabin(short weights, const weights_map* extra_weights)
{
	mesh M;
	if(weights == W_DEFAULT && extra_weights == NULL)
		ds_create_points_g(M);
	else if((weights == W_DEFAULT && extra_weights != NULL) ||
		 weights == W_CATMULL_CLARK)
		ds_create_points_p(M, mesh::ds_weights_cc, extra_weights);
	else if(weights == W_DOO_SABIN)
		ds_create_points_p(M, mesh::ds_weights_ds, extra_weights);
	else
		return;

	// Create new F-faces by connecting the appropriate vertex points
	// (generated above) of the face
	for(size_t i = 0; i < F.size(); i++)
	{
		// Since the vertex points are visited in the order of the old
		// vertices, this step is orientation-preserving

		std::vector<vertex*> vertices;
		for(size_t j = 0; j < F[i]->num_vertices(); j++)
			vertices.push_back(F[i]->get_face_vertex(j));

		M.add_face(vertices);
	}

	// Create quadrilateral E-faces
	for(std::vector<edge*>::iterator it = E.begin(); it != E.end(); it++)
	{
		edge* e = *it;

		// Skip border edges--we cannot create any new faces here
		if(e->get_g() == NULL)
			continue;

		/*
			The situation is as follows:

			---------- v ----------
			|          |          |
			|    F     |     G    |
			|          |          |
			|	   |          |
			---------- u ----------

			Since F is the first face that we encountered when
			traversing the edge in its natural direction, we know
			that the orientation is preserved if the corresponding
			face points are connected like:

			u_F -- u_G -- v_G -- v_F -- u_F

		*/

		vertex* v1 = find_face_vertex(e->get_f(), e->get_u());
		vertex* v2 = find_face_vertex(e->get_g(), e->get_u());
		vertex* v3 = find_face_vertex(e->get_g(), e->get_v());
		vertex* v4 = find_face_vertex(e->get_f(), e->get_v());

		M.add_face(v1, v2, v3, v4);
	}

	// Create V-faces by connecting the face vertices of all faces that are
	// adjacent to a fixed vertex.
	for(size_t i = 0; i < V.size(); i++)
	{
		// This is a quick fix required for processing some meshes that
		// are degenerate
		if(V[i]->num_adjacent_faces() < 3)
			continue;

		// The faces need to be sorted in counterclockwise order around
		// the vertex.
		std::vector<face*> faces = sort_faces(V[i]);

		std::vector<vertex*> vertices;
		for(size_t j = 0; j < V[i]->num_adjacent_faces(); j++)
			vertices.push_back(find_face_vertex(faces[j], V[i]));

		M.add_face(vertices);
	}

	this->replace_with(M);
}

/*!
*	Creates the new points of the Doo-Sabin subdivision scheme. This
*	function uses the geometrical approach as presented in the paper of Doo
*	and Sabin. User-configurable parameters are ignored.
*
*	@param M Mesh that stores the new points
*/

void mesh::ds_create_points_g(mesh& M)
{
	// Create new points
	for(size_t i = 0; i < F.size(); i++)
	{
		// Find centroid of face
		v3ctor centroid;
		for(size_t j = 0; j < F[i]->num_vertices(); j++)
		{
			const vertex* v = F[i]->get_vertex(j);
			centroid += v->get_position();
		}
		centroid *= 1.0/F[i]->num_vertices();

		// For a fixed vertex of the face, find the two edges that are
		// incident on this vertex and calculate their midpoints.
		for(size_t j = 0; j < F[i]->num_vertices(); j++)
		{
			const vertex* v = F[i]->get_vertex(j);

			const edge* e1 = NULL;
			const edge* e2 = NULL;
			for(size_t k = 0; k < F[i]->num_edges(); k++)
			{
				if(	F[i]->get_edge(k).e->get_u() == v ||
					F[i]->get_edge(k).e->get_v() == v)
				{
					if(e1 == NULL)
						e1 = F[i]->get_edge(k).e;
					else
					{
						e2 = F[i]->get_edge(k).e;
						break;
					}
				}
			}

			assert(e1 != NULL);
			assert(e2 != NULL);

			// Calculate midpoints of the edges and the position of
			// face vertex

			v3ctor midpoint1;
			v3ctor midpoint2;

			midpoint1 = (e1->get_u()->get_position()+e1->get_v()->get_position())/2;
			midpoint2 = (e2->get_u()->get_position()+e2->get_v()->get_position())/2;

			v3ctor face_vertex_position = (midpoint1+midpoint2+centroid+v->get_position())/4;

			// Add new vertex to the face. The lookup using the
			// vertex's ID is necessary because the face only
			// supplies const pointers.

			vertex* face_vertex = M.add_vertex(face_vertex_position);
			F[i]->add_face_vertex(face_vertex);
		}
	}
}

/*!
*	Creates the new points of the Doo-Sabin subdivision scheme. This
*	function uses a parametrical approach, thus making it possible for the
*	user to specify different weights in order to fine-tune the algorithm.
*
*	@param M		Mesh that stores the new points
*	@param weight_function	Pointer to weight function
*	@param extra_weights	Allows the user to override weights for some
*				faces. This means that for certain faces, the
*				user-defined weights are used instead of the
*				weights defined by the weight function.
*/

void mesh::ds_create_points_p(mesh& M, double (*weight_function)(size_t, size_t), const weights_map* extra_weights)
{
	// Only used if extra_weights has been defined
	weights_map::const_iterator it;
	std::vector<double> weights;

	for(std::vector<face*>::iterator f = F.begin(); f != F.end(); f++)
	{
		size_t k = (*f)->num_vertices();
		std::vector<const vertex*> vertices = sort_vertices(*f, (*f)->get_vertex(0));

		// Check if weights for a face with k vertices can be found
		if(	extra_weights != NULL &&
			((it = extra_weights->find(k)) != extra_weights->end()))
			weights = it->second;
		else
			weights.clear();

		for(size_t i = 0; i < vertices.size(); i++)
		{
			v3ctor face_vertex_position;

			// If a user-defined weights are present and weights for the current
			// number of vertices have been found
			if(!weights.empty())
			{
				for(size_t j = 0; j < weights.size(); j++)
					face_vertex_position += vertices[j]->get_position()*weights[j];
			}

			// Use weight distribution function
			else
			{
				for(size_t j = 0; j < vertices.size(); j++)
					face_vertex_position += vertices[j]->get_position()*weight_function(k,j);
			}

			vertex* face_vertex = M.add_vertex(face_vertex_position);
			(*f)->add_face_vertex(face_vertex);

			// Shift the vector

			const vertex* v = vertices[0];
			vertices.erase(vertices.begin());
			vertices.push_back(v);
		}
	}
}

/*!
*	Computes the weight factor for the i-th vertex of a face with k
*	vertices. The formula of Doo and Sabin is used.
*
*	This function only applies to the Doo-Sabin subdivision scheme.
*
*	@param i Index of vertex in face (0, 1, ..., k-1)
*	@param k Number of vertices
*
*	@return Weight
*/

inline double mesh::ds_weights_ds(size_t k, size_t i)
{
	if(i == 0)
		return(0.25+5.0/(4.0*k));
	else
		return((3.0+2.0*cos(2*M_PI*i/k))/(4.0*k));
}

/*!
*	Computes the weight factor for the i-th vertex of a face with k
*	vertices. The formula of Catmull and Clark is used.
*
*	This function only applies to the Doo-Sabin subdivision scheme.
*
*	@param i Index of vertex in face (0, 1, ..., k-1)
*	@param k Number of vertices
*
*	@return Weight
*/

inline double mesh::ds_weights_cc(size_t k, size_t i)
{
	if(i == 0)
		return(0.5+1.0/(4.0*k));
	else
	{
		if(i == 1 || i == (k-1))
			return(0.125+1.0/(4.0*k));
		else
			return(1.0/(4.0*k));
	}
}

/*!
*	Performs one step of Catmull-Clark subdivision on the current mesh.
*
*	@param weights		Parameter is ignored for now
*	@param extra_weights	Parameter is ignored for now
*/

void mesh::subdivide_catmull_clark(short weights, const weights_map* extra_weights)
{
	mesh M;

	// Create face points
	for(size_t i = 0; i < F.size(); i++)
	{
		v3ctor centroid;
		for(size_t j = 0; j < F[i]->num_vertices(); j++)
			centroid += F[i]->get_vertex(j)->get_position();

		centroid /= F[i]->num_vertices();

		F[i]->face_point = M.add_vertex(centroid);
	}

	// Create edge points
	for(std::vector<edge*>::iterator it = E.begin(); it != E.end(); it++)
	{
		edge* e = *it;
		v3ctor edge_point;

		// Border/crease edge: Use midpoint of edge for the edge point
		if(e->get_g() == NULL)
		{
			edge_point = (	e->get_u()->get_position()+
					e->get_v()->get_position())*0.5;
		}

		// Normal edge
		else
		{
			edge_point = (	e->get_u()->get_position()+
					e->get_v()->get_position()+
					e->get_f()->face_point->get_position()+
					e->get_g()->face_point->get_position())*0.25;
		}

		e->edge_point = M.add_vertex(edge_point);
	}

	// Create vertex points
	for(size_t i = 0; i < V.size(); i++)
	{
		// This follows the original terminology as described by
		// Catmull and Clark

		v3ctor Q;
		v3ctor R;
		v3ctor S;

		size_t n = V[i]->valency();
		if(n < 3)
			continue; // ignore degenerate vertices

		// Q is the average of the new face points of all faces
		// adjacent to the old vertex point
		for(size_t j = 0; j < V[i]->num_adjacent_faces(); j++)
			Q += V[i]->get_face(j)->face_point->get_position();

		Q /= V[i]->num_adjacent_faces();

		// R is the average of the midpoints of all old edges incident
		// on the current vertex
		for(size_t j = 0; j < n; j++)
		{
			const edge* e = V[i]->get_edge(j);
			R += (e->get_u()->get_position()+e->get_v()->get_position())*0.5;
		}

		R /= n;

		// S is the current vertex
		S = V[i]->get_position();

		v3ctor vertex_point =(Q+R*2+S*(n-3))/n;
		V[i]->vertex_point = M.add_vertex(vertex_point);
	}

	/*
		Create new topology of the mesh by connecting

			vertex -- edge -- face -- edge

		points.
	*/

	for(size_t i = 0; i < V.size(); i++)
	{
		if(V[i]->vertex_point == NULL)
			continue; // ignore degenerate vertices

		for(size_t j = 0; j < V[i]->num_adjacent_faces(); j++)
		{
			const face* f = V[i]->get_face(j);

			// Find the two incident edges of the current vertex
			// that are also part of the current adjacent face

			const edge* e1 = NULL;
			const edge* e2 = NULL;

			for(size_t k = 0; k < V[i]->valency(); k++)
			{
				const edge* e = V[i]->get_edge(k);
				if(e->get_f() == f || e->get_g() == f)
				{
					if(e1 == NULL)
						e1 = e;
					else
					{
						e2 = e;
						break;
					}
				}
			}

			/*
				Check which edge needs to be used first in
				order to orient the new face properly. The
				rationale behind this is to ensure that e1 is
				the _first_ edge that needs to be visited in
				order to get CCW orientation.
			*/

			if(	(e1->get_u()->get_id() == V[i]->get_id() && e1->get_g() == f) ||
				(e1->get_v()->get_id() == V[i]->get_id() && e1->get_f() == f) ||
				(e2->get_u()->get_id() == V[i]->get_id() && e2->get_f() == f) ||
				(e2->get_v()->get_id() == V[i]->get_id() && e2->get_g() == f))
				std::swap(e1, e2);

			M.add_face(	V[i]->vertex_point,
					e1->edge_point,
					f->face_point,
					e2->edge_point);
		}
	}

	this->replace_with(M);
}

/*!
*	Given an edge and a triangular face (where the edge is supposed to be
*	part of the face), return the remaining vertex of the face. This
*	function is used for Loop subdivision.
*
*	@param e Edge
*	@param f Face that is adjacent to the edge. The face is supposed to
*	have only 3 vertices.
*
*	@return Pointer to the remaining vertex of the face or NULL if the
*	vertex could not be found.
*/

const vertex* mesh::find_remaining_vertex(const edge* e, const face* f)
{
	const vertex* result = NULL;
	if(f == NULL || e == NULL)
		return(result);

	for(size_t i = 0; i < f->num_vertices(); i++)
	{
		// The IDs of the start and end vertices of the edge
		// differ from the ID of the remaining edge.
		if(	f->get_vertex(i)->get_id() != e->get_u()->get_id() &&
			f->get_vertex(i)->get_id() != e->get_v()->get_id())
		{
			result = f->get_vertex(i);
			break;
		}
	}

	return(result);
}

/*!
*	Given a vertex and a face (of which the vertex is assumed to be a
*	part), find the corresponding face vertex and return a pointer to it.
*
*	@param f Face
*	@param v Vertex, which is assumed to be a part of the face.
*
*	@return Pointer to the face vertex that corresponds to vertex v in the
*	face.
*/

vertex* mesh::find_face_vertex(face* f, const vertex* v)
{
	for(size_t i = 0; i < f->num_vertices(); i++)
	{
		// TODO: Speed could be increased by using lookup tables that map the "old" id to the "new id"
		if(f->get_vertex(i)->get_id() == v->get_id())
			return(f->get_face_vertex(i));
	}

	return(NULL);
}

/*!
*	Given a face and a vertex v, sort all vertices of the face in
*	counterclockwise order, starting with vertex v.
*
*	@param f Pointer to the face
*	@param v Pointer to the "first" vertex
*
*	@return Sorted vector of vertices
*/

std::vector<const vertex*> mesh::sort_vertices(face* f, const vertex* v)
{
	std::vector<const vertex*> vertices;

	size_t pos_v = std::numeric_limits<std::size_t>::max();
	directed_edge d_edge_v;

	for(size_t i = 0; i < f->num_edges(); i++)
	{
		d_edge_v = f->get_edge(i);
		if(	d_edge_v.e->get_u()->get_id() == v->get_id() ||
			d_edge_v.e->get_v()->get_id() == v->get_id())
		{
			pos_v = i;
			break;
		}
	}

	assert(pos_v != std::numeric_limits<std::size_t>::max());

	bool take_first = false;	// signals whether the first or the second
					// edge is to be taken for each edge
	if(d_edge_v.e->get_u()->get_id() == v->get_id())
	{
		vertices.push_back(d_edge_v.e->get_u());
		take_first = !d_edge_v.inverted;
	}
	else
	{
		vertices.push_back(d_edge_v.e->get_v());
		take_first = d_edge_v.inverted;
	}

	for(size_t i = pos_v; (i-pos_v) < f->num_edges(); i++)
	{
		const vertex* w;
		directed_edge d_e;

		// Index must wrap around once the end is reached
		if(i >= f->num_edges())
			d_e = f->get_edge(i-f->num_edges());
		else
			d_e = f->get_edge(i);

		// Always take the _same_ vertex of each edge (depending on the
		// position of v)
		if(d_e.inverted)
			w = (take_first ? d_e.e->get_v() : d_e.e->get_u());
		else
			w = (take_first ? d_e.e->get_u() : d_e.e->get_v());

		// Avoid duplicates. They can only occur with v because v will
		// always be the first vertex in the result vector regardless
		// of whether it was the first or second vertex of the edge
		if(w->get_id() != v->get_id())
			vertices.push_back(w);
	}

	return(vertices);
}

/*!
*	Given a vertex, sort all the vertex's adjacent faces in
*	counterclockwise order around the vertex.
*
*	@param v Pointer to vertex
*	@return Sorted vector of faces
*/

std::vector<face*> mesh::sort_faces(vertex* v)
{
	std::vector<face*> faces;
	std::vector<edge*> edges;

	for(size_t i = 0; i < v->valency(); i++)
		edges.push_back(v->get_edge(i));

	/*
		The vector of edges is sorted by the following considerations:

			(1)	All incident edges of the vertex are known
			(2)	By using the adjacent faces of an edge, a list of
				adjacent faces can be built:
				(2.1)	Start with any edge
				(2.2)	Search for any other edge which shares
					any face with the start edge. If the edge is
					found, store it after the start edge and repeat
					the process with the new edge.

		This works because there are only two edges which share one
		face. So the next edge that is found will _not_ point to any
		face that has already been processed (i.e., that will _not_ be
		touched by the algorithm anymore).
	*/

	for(size_t i = 0; i < edges.size(); i++)
	{
		for(size_t j = i+1; j < edges.size(); j++)
		{
			if(	edges[j]->get_f() == edges[i]->get_f() ||
				edges[j]->get_g() == edges[i]->get_g() ||
				edges[j]->get_f() == edges[i]->get_g() ||
				edges[j]->get_g() == edges[i]->get_f())
			{
				std::swap(edges[j], edges[i+1]);
				break;
			}
		}
	}

	/*
		From the sorted edges, the faces can be extracted in sorted
		order by checking which faces of the edges coincide: If edges
		e1 and e2 have adjacent faces (f1,f2), (f2,f3), f3 will be
		added to the face vector. The face vector has to be initialized
		with one face, which would be missing otherwise.
	*/

	faces.push_back(edges[0]->get_f());
	for(size_t i = 1; i < edges.size(); i++)
	{
		if(	edges[i]->get_f() == edges[i-1]->get_f() ||
			edges[i]->get_f() == edges[i-1]->get_g())
		{
			// Border edges are simply ignored
			if(edges[i]->get_g() != NULL)
				faces.push_back(edges[i]->get_g());
		}
		else
			faces.push_back(edges[i]->get_f());
	}

	// Check whether orientation is CW or CCW by enumerating all relevant
	// configurations.

	bool revert = false;
	if(edges[0]->get_u() == v)
	{
		if(faces[1] == edges[0]->get_g())
			revert = true;
	}
	else
	{
		if(faces[1] != edges[0]->get_g())
			revert = true;
	}

	if(revert)
		reverse(faces.begin(), faces.end());

	return(faces);
}

} // end of namespace "psalm"
