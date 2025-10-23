//#pragma once
//
//#include <string>
//
//namespace XEngine::graphics
//{
//	enum class TextureFilter
//	{
//		Nearest,
//		Linear
//	};
//
//	class Texture
//	{
//	public:
//		Texture(const std::string& path);
//		~Texture();
//
//		inline uint32_t getId() const { return mId; }
//		inline uint32_t getWidth() const { return mWidth; }
//		inline uint32_t getHeight() const { return mHeight; }
//		inline uint32_t getNumChannels() const { return mNumChannels; }
//		inline const std::string& GetPath() const { return mPath; }
//		inline TextureFilter GetTextureFilter() const { return mFilter; }
//
//		void bind();
//		void unbind();
//
//		void setTextureFilter(TextureFilter filter);
//
//	private:
//		void loadTexture();
//
//	private:
//		TextureFilter mFilter;
//
//		std::string mPath;
//		uint32_t mId;
//		uint32_t mWidth, mHeight;
//		uint32_t mNumChannels;
//
//		unsigned char* mPixels;
//	};
//}