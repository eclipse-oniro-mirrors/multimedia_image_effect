/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RENDER_RESOURCE_CACHE_H
#define RENDER_RESOURCE_CACHE_H

#include "base/render_base.h"
#include "base/cache/render_fifo_cache.h"
#include "graphic/render_general_program.h"
#include "render_mesh.h"
#include "graphic/render_texture.h"

#define TEXTURE_CACHE_MAX_CAPACITY (800 * 1024 * 1024)
#define TEXTURE_CACHE_STABLE_CAPACITY (80 * 1024 * 1024)

namespace OHOS {
namespace Media {
namespace Effect {
class RenderFrameBuffer;
using RenderFrameBufferPtr = std::shared_ptr<RenderFrameBuffer>;
class RenderEffectBase;
using RenderEffectBasePtr = std::shared_ptr<RenderEffectBase>;

constexpr int TEX_WIDTH_TAG_POS = 48;
constexpr int TEX_HEIGHT_TAG_POS = 32;
constexpr int RESIZE_RATE = 2;

class ResourceCache {
public:
    ~ResourceCache()
    {
        texReleaseFlag = true;
        DeleteAllShader();
        DeleteAllMesh();
    }
    RenderGeneralProgram *GetShader(const std::string name)
    {
        auto ite = shadersMap_.find(name);
        if (ite == shadersMap_.end()) {
            return nullptr;
        }
        return ite->second;
    }

    RenderMesh *GetMesh(const std::string name)
    {
        auto ite = meshesMap_.find(name);
        if (ite == meshesMap_.end()) {
            return nullptr;
        }
        return ite->second;
    }

    RenderEffectBasePtr GetEffect(const std::string name)
    {
        auto ite = effectMap_.find(name);
        if (ite == effectMap_.end()) {
            return nullptr;
        }
        return ite->second;
    }

    void AddShader(std::string name, RenderGeneralProgram *shader)
    {
        shadersMap_[name] = shader;
    }

    void AddMesh(std::string name, RenderMesh *mesh)
    {
        meshesMap_[name] = mesh;
    }

    void AddEffect(std::string name, RenderEffectBasePtr effect)
    {
        effectMap_[name] = effect;
    }

    size_t RemoveShader(std::string name)
    {
        return shadersMap_.erase(name);
    }

    size_t RemoveMesh(std::string name)
    {
        return meshesMap_.erase(name);
    }

    size_t RemoveEffect(std::string name)
    {
        return effectMap_.erase(name);
    }

    RenderTexturePtr RequestTexture(RenderContext *ctx, GLsizei w, GLsizei h, GLenum interFmt)
    {
        UINT64 tag = GetTexTag(w, h, interFmt);
        RenderTexture *rawTex;
        RenderTexturePtr tex;
        bool isGot = disuseTexCache_.Take(tag, tex);
        if (isGot) {
            rawTex = tex.get();
            tex.reset();
        } else {
            rawTex = new RenderTexture(ctx, w, h, interFmt);
            rawTex->Init();
        }
        return RenderTexturePtr(rawTex, [this](auto *p) {
            if (p) {
                RecycleTexture(dynamic_cast<RenderTexture *>(p));
            }
        });
    }

    void ResizeTexCache()
    {
        if (disuseTexCache_.Size() > TEXTURE_CACHE_STABLE_CAPACITY) {
            texReleaseFlag = true;
            disuseTexCache_.ReSize(disuseTexCache_.Size() / RESIZE_RATE, false);
            texReleaseFlag = false;
        }
    }

    void AddTexStage(int id, RenderTexturePtr tex)
    {
        namedTexCache_.insert_or_assign(id, tex);
    }

    RenderTexturePtr GetTexStage(int id)
    {
        auto ite = namedTexCache_.find(id);
        if (ite != namedTexCache_.end()) {
            return ite->second;
        }
        return nullptr;
    }

    void RemoveTexStage(int id)
    {
        auto ite = namedTexCache_.find(id);
        if (ite != namedTexCache_.end()) {
            namedTexCache_.erase(ite);
        }
    }

    void AddTexGlobalCache(std::string id, RenderTexturePtr tex)
    {
        texGlobalCache_.insert_or_assign(id, tex);
    }

    RenderTexturePtr GetTexGlobalCache(const std::string id)
    {
        auto ite = texGlobalCache_.find(id);
        if (ite != texGlobalCache_.end()) {
            return ite->second;
        }
        return nullptr;
    }

    void RemoveTexGlobalCache(const std::string id)
    {
        auto ite = texGlobalCache_.find(id);
        if (ite != texGlobalCache_.end()) {
            texGlobalCache_.erase(ite);
        }
    }

private:
    void RecycleTexture(RenderTexture *tex)
    {
        UINT64 tag = GetTexTag(tex->Width(), tex->Height(), tex->Format());
        auto func = [this](RenderTexture *p) {
            if (texReleaseFlag && p) {
                p->Release();
                delete p;
            }
        };
        texReleaseFlag = true;
        disuseTexCache_.Put(tag, RenderTexturePtr(tex, func));
        texReleaseFlag = false;
    }

    bool texReleaseFlag{ false };
    std::unordered_map<std::string, RenderGeneralProgram *> shadersMap_;
    std::unordered_map<std::string, RenderMesh *> meshesMap_;
    RenderFifoCache<UINT64, RenderTexturePtr, TextureSizeMeasurer> disuseTexCache_{TEXTURE_CACHE_MAX_CAPACITY};
    std::unordered_map<int, RenderTexturePtr> namedTexCache_;
    std::unordered_map<std::string, RenderTexturePtr> texGlobalCache_;
    std::unordered_map<std::string, RenderEffectBasePtr> effectMap_;

    void DeleteAllShader()
    {
        std::unordered_map<std::string, RenderGeneralProgram *>::iterator iter = shadersMap_.begin();
        while (iter != shadersMap_.end()) {
            iter->second->Release();
            iter++;
        }
        shadersMap_.clear();
    }

    void DeleteAllMesh()
    {
        std::unordered_map<std::string, RenderMesh *>::iterator iter = meshesMap_.begin();
        while (iter != meshesMap_.end()) {
            delete iter->second;
            iter->second = nullptr;
            meshesMap_.erase(iter++);
        }
    }

    UINT64 GetTexTag(GLsizei w, GLsizei h, GLenum interFmt)
    {
        return ((UINT64)interFmt & 0xffffffff) | (((UINT64)h & 0xffff) << TEX_HEIGHT_TAG_POS) |
            (((UINT64)w & 0xffff) << TEX_WIDTH_TAG_POS);
    }
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif