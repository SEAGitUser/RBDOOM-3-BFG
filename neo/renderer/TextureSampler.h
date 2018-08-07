#ifndef __TEXTURESAMPLER_H__
#define __TEXTURESAMPLER_H__

struct idTextureSampler {
	GLuint oglSamplerState;
};

class idUniqueTextureSampler 
{
	idUniqueTextureSampler();
	idUniqueTextureSampler( const samplerOptions_t & options_ );
	~idUniqueTextureSampler();

	idStr				name;
	uint32				numUsers;
	samplerOptions_t	options;
	idTextureSampler	sampler;
};

class idTextureSamplerManager {
public:
	void Init(/* options*/);
	void Shutdown();

	idTextureSampler *	FindSamplerByName( const char *name, bool addUser );
	const char *		GetSamplerName( idTextureSampler * sampler );
	idTextureSampler *	CreateSampler( const samplerOptions_t & options, const char *name );
	void				DeleteSampler( idTextureSampler * sampler );

	idUniqueTextureSampler pointSampler;
	idUniqueTextureSampler pointSamplerRepeating;
	idList<idUniqueTextureSampler> mipMapSamplers;
};

class idTextureSamplerManagerLocal : public idTextureSamplerManager {
	friend class idImage;
public:
	void Init( /*options*/ );
	void Shutdown();

	idTextureSampler *	FindSamplerByName( const char *name, bool addUser );
	const char *		GetSamplerName( idTextureSampler * sampler );
	idTextureSampler *	CreateSampler( const samplerOptions_t & options, const char *name );
	void				DeleteSampler( idTextureSampler * sampler );

private:
	idList<idUniqueTextureSampler, 5> uniqueTextureSamplers;
	idTextureSampler * FindSamplerByOptions( const samplerOptions_t & options, bool addUser );
} textureSamplerManagerLocal;

idTextureSamplerManagerLocal	textureSamplerManagerLocal;
idTextureSamplerManager	*		textureSamplerManager;

#endif /*__TEXTURESAMPLER_H__*/
