#include <GL/glew.h>
#include <GL/glfw.h>
#include <glm/glm.hpp>
#include <exception>
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <IL/il.h>
#include <string.h>
#include <cstdlib>

static const char *vs =
"#version 330 core\n"
"\n"
"layout (location = 0) in vec3 in_position;\n"
"layout (location = 1) in vec3 in_normal;\n"
"layout (location = 2) in vec2 in_texcoord;\n"
"out vec3 pass_position;\n"
"out vec3 pass_normal;\n"
"out vec2 pass_texcoord;\n"
"\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(in_position, 1.0);\n"
"  pass_position = in_position;\n"
"  pass_normal = in_normal;\n"
"  pass_texcoord = in_texcoord;\n"
"}\n";

static const char *fs =
"#version 330 core\n"
"\n"
"uniform sampler2D modeltex;\n"
"in vec3 pass_position;\n"
"in vec3 pass_normal;\n"
"in vec2 pass_texcoord;\n"
"out vec3 outColor;\n"
"\n"
"void main(void)\n"
"{\n"
"  outColor = texture2D(modeltex, pass_texcoord).xyz;\n"
"}\n";

class GeneralException : public std::exception
{
public:
	GeneralException(const std::string &errors)
	: errors(errors)
	{
	}
	~GeneralException() throw ()
	{
	}
	const char *what() const throw()
	{
		return errors.c_str();
	}
private:
	const std::string errors;
};

struct Lump
{
	size_t size;
	std::unique_ptr<unsigned char> buf;
};

class File
{
public:
	FILE* file;
	File(const std::string name, const char* mode)
	: file(fopen(name.c_str(), mode))
	{
		if(file == 0) throw GeneralException("could not open file");
	}
	~File()
	{
		fclose(file);
	}
	static Lump read(const std::string name)
	{
		File file(name, "r");
		fseek(file.file, 0L, SEEK_END);
		Lump lump;
		lump.size = ftell(file.file);
		rewind(file.file);
		unsigned char* buf = (unsigned char*)malloc(lump.size);
		lump.buf.reset(buf);
		size_t done = 0;
		do {
			size_t read = fread(buf+done, 1, lump.size - done, file.file);
			assert(read != 0);
			done += read;
		} while(done != lump.size);
		return lump;
	}
	static void write(const std::string name, const Lump& lump)
	{
		File file(name, "w");
		size_t done = 0;
		do {
			size_t written = fwrite(lump.buf.get()+done, 1, lump.size - done, file.file);
			assert(written != 0);
			done += written;
		} while(done != lump.size);
	}
};

class VertexBuffer
{
public:
	struct InputElementDescription
	{
		std::string name;
		size_t numberofelements;
		size_t elementsize;
	};
	template <typename T>
	VertexBuffer(InputElementDescription *description, const T *vertexData, size_t count)
	: stride(sizeof(T))
	, count(count)
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &id);
		glBindBuffer(GL_ARRAY_BUFFER, id);
		size_t offset = 0;
		for(int i = 0; description[i].elementsize; ++i)
		{
			glEnableVertexAttribArray(i);
			glVertexAttribPointer(i, description[i].numberofelements, GL_FLOAT, GL_FALSE, stride, (void *)offset);
			offset += description[i].elementsize;
		}
		glBufferData(GL_ARRAY_BUFFER, stride * count, vertexData, GL_STATIC_DRAW);
	}
	void Draw()
	{
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, count);
	}
	~VertexBuffer()
	{
		glDeleteBuffers(1, &id);
		glDeleteVertexArrays(1, &vao);
	}
private:
	unsigned int id;
	unsigned int vao;
	unsigned int stride;
	size_t count;
};

class Shader
{
public:
	Shader(const char* source, int type) : type(type)
	{
		id = glCreateShader(type);
		glShaderSource(id, 1, &source, 0);
		glCompileShader(id);
		int ok = true;
		glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
		if(!ok)
		{
			int length = 0;
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
			std::unique_ptr<char> errors(new char[length]);
			glGetShaderInfoLog(id, length, 0, errors.get());
			throw GeneralException(std::string(errors.get()));
		}
	}
	virtual ~Shader()
	{
		glDeleteShader(id);
	}
	unsigned int GetId() { return id; }
private:
	unsigned int id;
	int type;
};

class VertexShader : public Shader
{
public:
	VertexShader(const char* source) : Shader(source, GL_VERTEX_SHADER) {}
};

class FragmentShader : public Shader
{
public:
	FragmentShader(const char* source) : Shader(source, GL_FRAGMENT_SHADER) {}
};

class ShaderProgram
{
public:
	ShaderProgram(VertexShader &vertexShader, FragmentShader &fragmentShader, VertexBuffer::InputElementDescription* description)
	: vertexShader(vertexShader)
	, fragmentShader(fragmentShader)
	{
		id = glCreateProgram();
		if(id == 0) throw GeneralException("glCreateProgram of shader failed");
		glAttachShader(id, vertexShader.GetId());
		glAttachShader(id, fragmentShader.GetId());

		for(int i = 0; description[i].elementsize; ++i)
		{
			glBindAttribLocation(id, i, description[i].name.c_str());
		}
		glLinkProgram(id);

		int ok = true;
		glGetShaderiv(id, GL_LINK_STATUS, &ok);
		glGetError(); //TODO: remove this because it is used to ignore the error GL_INVALID_OPERATION after glGetShaderiv
		if(!ok)
		{
			int length = 0;
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
			std::unique_ptr<char> errors(new char[length]);
			glGetProgramInfoLog(id, length, 0, errors.get());
			throw GeneralException(std::string(errors.get()));
		}
	}
	~ShaderProgram()
	{
		glDetachShader(id, fragmentShader.GetId());
		glDetachShader(id, vertexShader.GetId());
		glDeleteProgram(id);
	}
	void Set(const char *name, const float mat[16])
	{
		int uniform = glGetUniformLocation(id, name);
		if (uniform == -1) return;
		glUseProgram(id);
		glUniformMatrix4fv(uniform, 1, GL_FALSE, mat);
		int error = glGetError(); if (error) { printf("%d\n", error); abort(); }
	}
	bool SetTexture(const char *name, int value)
	{
		int error;
		int uniform = glGetUniformLocation(id, name);
		error = glGetError(); if (error) { printf("%d\n", error); abort(); }
		if (uniform == -1) return false;
		glUseProgram(id);
		glUniform1i(uniform, value);
		error = glGetError(); if (error) { printf("%d\n", error); abort(); }
		return true;
	}
	void Use()
	{
		glUseProgram(id);
	}
private:
	unsigned int id;
	VertexShader& vertexShader;
	FragmentShader& fragmentShader;
};

class Texture
{
	friend class Image;
public:
	struct ImageData
	{
		int type;
		Lump lump;
	};
	class Image
	{
	private:
		Texture& texture;
		unsigned int id;
	public: 
		Image(Texture& texture)
		: texture(texture)
		{
			ilGenImages(1, &id);
			ilBindImage(id);
			ilTexImage(texture.width, texture.height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, 0);
		}
		void load(ImageData& imagedata)
		{
			ilBindImage(id);
			texture.Bind(0);
			ilLoadL(imagedata.type, imagedata.lump.buf.get(), imagedata.lump.size);
			assert(ilGetData() != 0);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.width, texture.height, 0, GL_RGB, GL_UNSIGNED_BYTE, ilGetData());
		}
		ImageData save()
		{
			const int maxwidth = 2048;
			const int maxheight = 2048;
			static unsigned char data[maxwidth * maxheight * 3];
			ilBindImage(id);
			texture.Bind(0);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, &data);
			assert(ilSetData(data));
			ImageData imagedata;
			imagedata.type = IL_PNG;
			imagedata.lump.size = ilSaveL(IL_PNG, data, maxwidth * maxheight * 3);
			unsigned char* savedata = (unsigned char*)malloc(imagedata.lump.size);
			imagedata.lump.buf.reset(savedata);
			memcpy(savedata, data, imagedata.lump.size);
			return imagedata;
		}
		~Image()
		{
			ilDeleteImages(1, &id);
		}
	};
private:
	unsigned int width;
	unsigned int height;
	Image image;
	unsigned int id;
public:
	Texture(unsigned int width, unsigned int height)
	: width(width)
	, height(height)
	, image(*this)
	{
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	}
	Texture(unsigned int width, unsigned int height, ImageData& imagedata)
	: width(width)
	, height(height)
	, image(*this)
	{
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		load(imagedata);
	}
	// Bind for render onto primitive.
	void Bind(int textureUnit)
	{
		glActiveTexture(GL_TEXTURE0+textureUnit);
		glBindTexture(GL_TEXTURE_2D, id);
        }
	// Attach for render to texture using fbo
	void Attach(unsigned int nr)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + nr, id, 0);
	}
	void load(ImageData& imagedata)
	{
		image.load(imagedata);
	}
	ImageData save()
	{
		return image.save();
	}
	~Texture()
	{
		glDeleteTextures(1, &id);
	}
};

static const unsigned int Attachments[] =
{
	GL_COLOR_ATTACHMENT0,
	GL_COLOR_ATTACHMENT1,
	GL_COLOR_ATTACHMENT2,
	GL_COLOR_ATTACHMENT3,
	GL_COLOR_ATTACHMENT4,
	GL_COLOR_ATTACHMENT5,
	GL_COLOR_ATTACHMENT6,
	GL_COLOR_ATTACHMENT7,
	GL_COLOR_ATTACHMENT8,
	GL_COLOR_ATTACHMENT9,
	GL_COLOR_ATTACHMENT10,
	GL_COLOR_ATTACHMENT11,
	GL_COLOR_ATTACHMENT12,
	GL_COLOR_ATTACHMENT13,
	GL_COLOR_ATTACHMENT14,
	GL_COLOR_ATTACHMENT15
};

class RenderTarget
{
public:
	RenderTarget(unsigned int width, unsigned int height, std::vector<Texture*> targets)
	: width(width)
	, height(height)
	{
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
		if(targets.size() > 0)
		{
			glDrawBuffers(targets.size(), Attachments);
			for(unsigned int i=0; i < targets.size(); i++)
			{
				targets[i]->Attach(i);
			}
		}
	}
	void Activate()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	~RenderTarget()
	{
		glDeleteRenderbuffers(1, &rbo);
		glDeleteFramebuffers(1, &fbo);
	}
private:
	unsigned int fbo, rbo;
	unsigned int width, height;
};

class RenderStep
{
public:
};


class Window_
{
public:
	Window_(size_t width, size_t height)
	{
		ilInit();
		if(!glfwInit()) throw new GeneralException("glfwInit failed");
		glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
		glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
		glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		if(!glfwOpenWindow(width, height, 8, 8, 8, 0, 24, 8, GLFW_WINDOW))
		{
			glfwTerminate();
			throw new GeneralException("glfwOpenWindow failed");
		}
		glewExperimental = GL_TRUE;
		if(glewInit() != GLEW_OK) throw new GeneralException("glewInit failed");
		glGetError(); // mask error of failed glewInit http://www.opengl.org/discussion_boards/ubbthreads.php?ubb=showflat&Number=284912
		glCullFace(GL_BACK);
		glFrontFace(GL_CW);
		glEnable(GL_CULL_FACE);
		glClearDepth(1.0f);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glViewport(0, 0, width, height);
	}
	void Swap()
	{
		glfwSwapBuffers();
	}
	~Window_()
	{
		glfwTerminate();
	}
};

int main()
{
	unsigned int width = 1024;
	unsigned int height = 768;
	Window_ window(width, height);
	VertexShader vertexShader(vs);
	FragmentShader fragmentShader(fs);
	static VertexBuffer::InputElementDescription description[] = {
		{ "in_position", 3, sizeof(glm::vec3) },
		{ "in_normal", 3, sizeof(glm::vec3) },
		{ "in_texcoord", 2, sizeof(glm::vec2) },
		{ "", 0, 0 }
	};
	ShaderProgram shaderProgram(vertexShader, fragmentShader, description);
	shaderProgram.Use();
	struct BufferInput
	{
		float x,y,z,nx,ny,nz,u,v;
	};
	BufferInput triangle[] = { { -1, -1, 1,  0,  0, 1,  0,  0}, {-1,  1, 1,  0,  0, 1,  0,  1}, {1, -1, 1,  0,  0, 1,  1,  0}, {-1,  1, 1,  0,  0, -1,  0,  1}, {1,  1, 1,  0,  0, -1,  1,  1}, {1, -1, 1,  0,  0, -1,  1,  0 }};

	VertexBuffer vb(description, triangle, sizeof(triangle)/sizeof(float));

	Texture texture(width, height);
	std::vector<Texture*> textures;
	textures.push_back(&texture);
	RenderTarget target(width, height, textures);

	Texture::ImageData imagedata({IL_PNG, File::read("pic.png")});	
	Texture pic(1024, 768, imagedata);

	bool running = true;
	while(running)
	{
		pic.Bind(0);
		shaderProgram.SetTexture("modeltex", 0);
		target.Activate();
		vb.Draw();
		window.Swap();
		running = !glfwGetKey(GLFW_KEY_ESC) &&
		          glfwGetWindowParam(GLFW_OPENED);

	}
	File::write("rendtex.png", texture.save().lump);
	return 0;
}

