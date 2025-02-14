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

#ifndef RENDER_DEFAULT_DATA_H
#define RENDER_DEFAULT_DATA_H

#include "base/render_base.h"

namespace OHOS {
namespace Media {
namespace Effect {
constexpr int RGBA_SIZE_PER_PIXEL = 4;

const std::vector<std::vector<float>> DEFAULT_VERTEX_DATA = { { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f } };
const std::vector<std::vector<float>> DEFAULT_FLIP_VERTEX_DATA = { { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f,
    1.0f, 0.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f } };

constexpr const char *DEFAULT_VERTEX_SHADER_SCREEN_CODE = "attribute vec4 aPosition;\n"
    "attribute vec4 aTextureCoord;\n"
    "varying vec2 textureCoordinate;\n"
    
    "void main()\n"
    "{\n"
    "    gl_Position = aPosition;\n"
    "    textureCoordinate = aTextureCoord.xy;\n"
    "}\n";

constexpr const char *TRANSFORM_VERTEX_SHADER_SCREEN_CODE = "attribute vec4 aPosition;\n"
    "attribute vec4 aTextureCoord;\n"
    "varying vec2 textureCoordinate;\n"
    "uniform mat4 transform;\n"
    
    "void main()\n"
    "{\n"
    "    gl_Position = transform * aPosition;\n"
    "    textureCoordinate = aTextureCoord.xy;\n"
    "}\n";

constexpr const char *DEFAULT_YUV_VERTEX_SHADER = "#version 310 es\n"
    "precision highp float;\n"
    "layout (location = 0) in vec3 aPosition;\n"
    "layout (location = 1) in vec2 aTextureCoord;\n"
    "out vec2 textureCoordinate;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(aPosition, 1.0);\n"
    "    textureCoordinate = aTextureCoord.xy;\n"
    "}\n";

constexpr const char *TRANSFORM_YUV_VERTEX_SHADER = "#version 310 es\n"
    "precision highp float;\n"
    "layout (location = 0) in vec3 aPosition;\n"
    "layout (location = 1) in vec2 aTextureCoord;\n"
    "out vec2 textureCoordinate;\n"
    "uniform mat4 transform;\n"
    "uniform float flipH;\n"
    "uniform float flipV;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = transform * vec4(aPosition, 1.0);\n"
    "    if (flipH > 0.0) {\n"
    "        textureCoordinate = vec2(1.0 - aTextureCoord.x, aTextureCoord.y);\n"
    "    } else if (flipV > 0.0) {\n"
    "        textureCoordinate = vec2(aTextureCoord.x, 1.0 - aTextureCoord.y);\n"
    "    } else {\n"
    "        textureCoordinate = aTextureCoord.xy;\n"
    "    }\n"
    "}\n";

constexpr const char *DEFAULT_YUV_SHADER_CODE = "#version 310 es\n"
    "#extension GL_EXT_YUV_target : require\n"
    "#extension GL_OES_EGL_image_external_essl3 : require\n"
    "precision highp float;\n"
    "uniform samplerExternalOES yuv_img_tex;\n"
    "in vec2 textureCoordinate;\n"
    "layout(yuv) out vec4 v_out_color;\n"
    "void main()\n"
    "{\n"
    "    vec4 inputrgb = texture(yuv_img_tex, textureCoordinate).rgba;\n"
    "    v_out_color = vec4(rgb_2_yuv(inputrgb.rgb, itu_601), 1.0);\n"
    "}\n";

constexpr const char *DEFAULT_YUV_RGBA_SHADER_CODE = "#version 310 es\n"
    "#extension GL_EXT_YUV_target : require\n"
    "#extension GL_OES_EGL_image_external_essl3 : require\n"
    "precision highp float;\n"
    "uniform samplerExternalOES yuv_img_tex;\n"
    "in vec2 textureCoordinate;\n"
    "out vec4 v_out_color;\n"
    "void main()\n"
    "{\n"
    "    vec3 inputrgb = texture(yuv_img_tex, textureCoordinate).rgb;\n"
    "    v_out_color = vec4(inputrgb, 1.0);\n"
    "}\n";

constexpr const char *DEFAULT_RGBA_YUV_SHADER_CODE = "#version 310 es\n"
    "#extension GL_EXT_YUV_target : require\n"
    "#extension GL_OES_EGL_image_external_essl3 : require\n"
    "precision highp float;\n"
    "uniform sampler2D inputTexture;\n"
    "in vec2 textureCoordinate;\n"
    "layout(yuv) out vec4 v_out_color;\n"
    "void main()\n"
    "{\n"
    "    vec3 inputrgb = texture(inputTexture, textureCoordinate).rgb;\n"
    "    v_out_color = vec4(rgb_2_yuv(inputrgb, itu_601), 1.0);\n"
    "}\n";

constexpr const char *DEFAULT_FRAGMENT_SHADER_CODE = "precision highp float;\n"
    "varying vec2 textureCoordinate;\n"
    "uniform sampler2D inputTexture;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = texture2D(inputTexture, textureCoordinate);\n"
    "}\n";

constexpr const char *DEFAULT_FRAGMENT_BGRA_SHADER_CODE = "precision highp float;\n"
    "varying vec2 textureCoordinate;\n"
    "uniform sampler2D inputTexture;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = texture2D(inputTexture, textureCoordinate).bgra;\n"
    "}\n";
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif