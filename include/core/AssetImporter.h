#ifndef ASSET_IMPORTER_H
#define ASSET_IMPORTER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace JJM {
namespace Core {

enum class AssetType {
    MESH, TEXTURE, AUDIO, ANIMATION, MATERIAL, SHADER, FONT, UNKNOWN
};

struct ImportSettings {
    bool flipUVs = false;
    bool generateNormals = false;
    bool generateTangents = false;
    bool mergeVertices = true;
    bool optimizeMesh = true;
    float scale = 1.0f;
    int maxTextureSize = 4096;
    bool generateMipmaps = true;
};

class AssetImporter {
public:
    virtual ~AssetImporter() = default;
    virtual bool canImport(const std::string& extension) const = 0;
    virtual void* import(const std::string& filepath, const ImportSettings& settings) = 0;
    virtual bool exportAsset(void* asset, const std::string& filepath) = 0;
};

class AssetPipeline {
public:
    AssetPipeline();
    ~AssetPipeline();
    
    void registerImporter(const std::string& extension, std::shared_ptr<AssetImporter> importer);
    void* importAsset(const std::string& filepath, const ImportSettings& settings = ImportSettings());
    bool exportAsset(void* asset, AssetType type, const std::string& filepath);
    
    static AssetType detectType(const std::string& filepath);
    
private:
    std::unordered_map<std::string, std::shared_ptr<AssetImporter>> m_importers;
};

} // namespace Core
} // namespace JJM

#endif
