

# ImageEffect框架

- [简介](#简介)
  - [基本概念](#基本概念)
- [目录](#目录)
- [编译](#编译)
- [使用说明](#使用说明)
  - [滤镜链处理](#滤镜链处理)
  - [单个滤镜处理](#单个滤镜处理)
  - [自定义滤镜处理](#自定义滤镜处理)

## 简介

图像效果引擎框架提供支持图片编辑业务的开发，开发者可以通过已开放的接口实现图片编辑相关功能的开发。框架提供了单个滤镜处理能力、滤镜链处理能力以及开发者自定义滤镜的处理能力，支持pixelmap、uri、surface等多种输入输出图片数据场景。

**图** 1 图片编辑组件架构图

![](figures/imageeffect-architecture_zh.jpg)

## 功能介绍
-   滤镜
    开发者可以通过滤镜的接口快速实现基本的效果处理，也可以将滤镜添加到效果器中，组合成滤镜链串联执行。系统提供了如”亮度“、”裁剪“等基本的效果处理滤镜。

-   效果器
    效果器提供了滤镜的添加、删除、查询等功能。开发者可以通过效果器提供的接口将多个滤镜组合串联，从而实现较为复杂的效果调节功能。同时，效果器支持多种输入类型，如pixelmap、uri、surface。不同的输入类型在效果器内部都会转换为内存对象，通过滤镜的效果处理，获得处理结果。

-   自定义效果
    开发者可以使用自定义滤镜实现定制化的滤镜效果，可以将自定义滤镜通过效果器接口添加到效果器中，与其他滤镜组合成滤镜链，也可以只单独使用自定义滤镜处理图片，获得处理效果。

## 目录

仓目录结构如下：

```
/foundation/multimedia/image_effect      # 图片编辑框架业务代码
├── frameworks                           # 框架代码
│   ├── native                           # 内部接口实现
│   │   └── capi                         # 接口实现
│   │   ├── effect                       # 效果类
│   │   └── efilter                      # 效果器实现
│   │   └── utils                        # 工具类
├── interfaces                           # 接口代码
│   ├── inner_api                        # 内部接口
│   └── kits                             # 外部接口
├── test                                 # 测试代码
│   └── unittest                         # 单元测试
├── BUILD.gn                             # 构建配置
├── bundle.json                          # 部件配置
├── config.gni                           # 构建参数配置
└── LICENSE                              # 证书文件
```

## 编译

```
./build.sh --product-name {product-name} --build-target foundation/multimedia/image_effect:image_effect
```

 {product-name}为当前支持的平台，比如rk3568.

## 使用说明

### 滤镜链处理

标准滤镜使用步骤:

1.  初始化
    
    ```
    napi_value result = nullptr;
    napi_get_undefined(env,&result);
    size_t argc = 2;
napi_value args[2] = {nullptr};
    
    napi_status status = napi_get_cb_info(env,info,&argc,args,nullptr,nullptr);
    CHECK_AND_RETURN_LOG(status == napi_ok, result, "napi_get_cb_info fail! status = %{public}d" ,status);
    
    NativePixelMap *inputPixel = OH_PixelMap_InitNativePixelMap(env,args[0]);
    NativePixelMap *outPixel = OH_PixelMap_InitNativePixelMap(env,args[1]);
    ```
    
    
    
2.  示例代码：创建ImageEffect对象，“imageEdit”为ImageEffect对象别名

    ```
       OH_ImageEffect *imageEffect = OH_ImageEffectCreate("imageEdit");
       CHECK_AND_RETURN_LOG(imageEffect != nullptr, result, "OH_ImageEffect_Create fail!");
    ```
3. 示例代码：将滤镜添加到ImageEffect容器中，可以添加多个滤镜，返回创建EFilter滤镜对象

   ```
   OH_EFilter *filter = OH_ImageEffect_AddFilter(imageEffect,"Brightness");
   CHECK_AND_RETURN_LOG(filter != nullptr, result, "OH_ImageEffect_AddFilter fail!");
   ```

4. 示例代码：设置滤镜参数

   ```
   OH_Any value{
         .dataType = OH_DataType::TYPE_FLOAT,
         .dataValue.floatValue = 100.f
    };
   OH_EffectErrorCode errorCode = OH_EFilter_SetValue(filter, "FILTER_INTENSITY", &value);
   CHECK_AND_RETURN_LOG(errorCode == OH_EffectErrorCode::EFFECT_ERR_SUCCESS, result, "OH_EFilter_SetValue fail, errorCode = %   {public}d" ,errorCode);
   ```

   

5. 示例代码：设置待处理的输入pixelMap

   ```
   errorCode = OH_ImageEffect_SetInputPixelMap(imageEffect, inputPixel);
   CHECK_AND_RETURN_LOG(errorCode == OH_EffectErrorCode::EFFECT_ERR_SUCCESS, result, "OH_ImageEffect_SetInputPixelMap fail, errorCode = %{public}d" ,errorCode);
   ```

   

6. 示例代码：设置待处理的输出pixelMap,可以不设置，当不设置时会直接在输入pixelMap上进行滤镜处理

   ```
   errorCode = OH_ImageEffect_SetOutputPixelMap(imageEffect, outPixel);
   CHECK_AND_RETURN_LOG(errorCode == OH_EffectErrorCode::EFFECT_ERR_SUCCESS, result, "OH_ImageEffect_SetOutputPixelMap fail, errorCode = %{public}d" ,errorCode);
   ```

   

7. 示例代码：执行生效滤镜算法

   ```
   errorCode = OH_ImageEffect_Start(imageEffect);
   CHECK_AND_RETURN_LOG(errorCode == OH_EffectErrorCode::EFFECT_ERR_SUCCESS, result, "OH_ImageEffect_Start fail, errorCode = %{public}d" ,errorCode);
   ```

   

8. 示例代码：释放资源

   ```
   OH_ImageEffect_Release(imageEffect);
   return result;
   ```

### 单个滤镜处理

1. 初始化

   ```
   napi_value result = nullptr;
   napi_get_undefined(env,&result);
   size_t argc = 2;
   napi_value args[2] = {nullptr};
   
   napi_status status = napi_get_cb_info(env,info,&argc,args,nullptr,nullptr);
   CHECK_AND_RETURN_LOG(status == napi_ok, result, "napi_get_cb_info fail! status = %{public}d" ,status);
   
   NativePixelMap *inputPixel = OH_PixelMap_InitNativePixelMap(env,args[0]);
   NativePixelMap *outPixel = OH_PixelMap_InitNativePixelMap(env,args[1]);
   
   napi_value result = nullptr;
   napi_get_undefined(env,&result);
   size_t argc = 2;
   napi_value args[2] = {nullptr};
   
   napi_status status = napi_get_cb_info(env,info,&argc,args,nullptr,nullptr);
   CHECK_AND_RETURN_LOG(status == napi_ok, result, "napi_get_cb_info fail! status = %{public}d" ,status);
   
   NativePixelMap *inputPixel = OH_PixelMap_InitNativePixelMap(env,args[0]);
   NativePixelMap *outPixel = OH_PixelMap_InitNativePixelMap(env,args[1]);
   ```

   

2. 示例代码：创建EFiter滤镜对象，"Brightness"为滤镜名

   ```
   OH_EFilter *filter = OH_EFilter_Create("Brightness");
   CHECK_AND_RETURN_LOG(filter != nullptr, result, "OH_EFilter_Create fail!");
   ```

3. 示例代码：设置滤镜参数

   ```
   OH_Any value{
          .dataType = OH_DataType::TYPE_FLOAT,
          .dataValue.floatValue = 100.f
      };
   OH_EffectErrorCode errorCode = OH_EFilter_SetValue(filter, "FILTER_INTENSITY", &value);
   CHECK_AND_RETURN_LOG(errorCode == OH_EffectErrorCode::EFFECT_ERR_SUCCESS, result, "OH_EFilter_SetValue fail, errorCode = %{public}d" ,errorCode);
   ```

4. 示例代码：执行生效滤镜算

   ```
   errorCode = OH_EFilter_Render(filter, inputPixel, outPixel);
   CHECK_AND_RETURN_LOG(errorCode == OH_EffectErrorCode::EFFECT_ERR_SUCCESS, result, "OH_EFilter_Render fail, errorCode = %{public}d" ,errorCode);
   ```

5. 示例代码：释放资源

   ```
   OH_ImageEffect_Release(imageEffect)
   return result;
   ```

### 自定义滤镜处理

1. 初始化

   ```
   napi_value result = nullptr;
   napi_get_undefined(env,&result);
   size_t argc = 2;
   napi_value args[2] = {nullptr};
   
   napi_status status = napi_get_cb_info(env,info,&argc,args,nullptr,nullptr);
   CHECK_AND_RETURN_LOG(status == napi_ok, result, "napi_get_cb_info fail! status = %{public}d" ,status);
   
   NativePixelMap *inputPixel = OH_PixelMap_InitNativePixelMap(env,args[0]);
   NativePixelMap *outPixel = OH_PixelMap_InitNativePixelMap(env,args[1]);
   ```

2. 自定义算子实现接口

   ```
   OH_EFilterDelegate delegate = {
        .setValue = [](OH_EFilter *filter, const *key,
        const OH_Any *value){return SetValue(filter, key, value);},
        .render = [](OH_EFilter *filter, OH_EffectBuffer *src,
        OH_EFilterDelegate_PushData pushData){return Apply(filter, src, pushData);},
        .save = [](OH_EFilter *filter, char **info){return Save(filter, info);},
        .restore = [](const char *info){return Restore(info);}
    };
   ```

3. 自定义算子信息

   ```
   OH_EffectInfo effectInfo = {
        .name = "CustomBrightness",
        .category = OH_Category::COLOR_ADJUST,
        .bufferTypes = OH_BufferType::PIXEL,
        .formats = OH_IEffectFormat::RGBA8888
    }
   ```

4. 示例代码：注册自定义滤镜算子

   ```
   OH_EffectErrorCode errorCode = OH_EFilter_Register(&effectInfo, &delegate);
   CHECK_AND_RETURN_LOG(errorCode == OH_EffectErrorCode::EFFECT_ERR_SUCCESS, result, "OH_EFilter_Register fail, errorCode = %{public}d" ,errorCode);
   ```

5. 示例代码：创建ImageEffect对象，“imageEdit”为ImageEffect对象别名

   ```
   OH_ImageEffect *imageEffect = OH_ImageEffectCreate("imageEdit");
   CHECK_AND_RETURN_LOG(imageEffect != nullptr, result, "OH_ImageEffect_Create fail!");
   ```

6. 示例代码：将滤镜添加到ImageEffect容器中，可以添加多个滤镜，返回创建EFilter滤镜对象

   ```
     OH_EFilter *filter = OH_ImageEffect_AddFilter(imageEffect,"CustomBrightness");
     CHECK_AND_RETURN_LOG(filter != nullptr, result, "OH_ImageEffect_AddFilter fail!");
   ```

7. 示例代码：设置滤镜参数

   ```
   OH_Any value{
       .dataType = OH_DataType::TYPE_FLOAT,
       .dataValue.floatValue = 100.f
   };
   errorCode = OH_EFilter_SetValue(filter, "brightness", &value);
   CHECK_AND_RETURN_LOG(errorCode == OH_EffectErrorCode::EFFECT_ERR_SUCCESS, result, "OH_EFilter_SetValue fail, errorCode = %{public}d" ,errorCode);
   ```

8. 示例代码：执行生效滤镜算法

   ```
   errorCode = OH_Efilter_Render(filter, inputPixel, outPixel );
   CHECK_AND_RETURN_LOG(errorCode == OH_EffectErrorCode::EFFECT_ERR_SUCCESS, result, "OH_Efilter_Render fail, errorCode = %{public}d" ,errorCode);
   ```

9. 示例代码：释放资源

   ```
   OH_ImageEffect_Release(imageEffect);
   return result;
   ```

10. 自定义算子接口实现示例

    ```
    static bool SetValue(OH_EFilter *filter, const *key, const OH_Any *value);
    
    // 自定义滤镜参数校验
    static bool SetValue(OH_EFilter *filter, const *key, const OH_Any *value)
    {
        //参数校验
        CHECK_AND_RETURN_LOG(std::string("brightness").compare(key) ==0, false,"key is not match! key=%{public}s" key);
        CHECK_AND_RETURN_LOG(value->dataType == OH_DataType::TYPE_FLOAT, false,"dataType is not int! key=%{public}d" value->dataType);
        float brightness = value->dataValue.floatValue;
        CHECK_AND_RETURN_LOG(brightness <= 100.f && brightness >= -00.f, false," brightness is out range! brightness=%{public}f" brightness);
        return true;
    }
    ```

    ```
    static bool Apply(OH_EFilter *filter, OH_EffectBuffer *src, OH_EFilterDelegate_PushData pushData);
    
    //执行自定义滤镜
    static bool Apply(OH_EFilter *filter, OH_EffectBuffer *src, OH_EFilterDelegate_PushData pushData)
    {
        // 获取滤镜算子需要的参数
        OH_Any value;
        OH_EffectErrorCode errorCode = OH_EFilter_GetValue(filter, "brightness", &value)
        CHECK_AND_RETURN_LOG(errorCode == OH_EffectErrorCode::EFFECT_ERR_SUCCESS && value.dataType == OH_DataType::TYPE_FLOAT, result, "OH_EFilter_GetValue fail, errorCode = %{public}d" ,errorCode);
        
        CustomAdjustBrightness(src, src, value.dataValue.floatValue);
        
        pushData(filter, src);
        return true;
    }
    ```

    ```
    static bool Save(OH_EFilter *filter, char **info);
    
    // 序列化滤镜
    static bool Save(OH_EFilter *filter, char **info){
        // 获取滤镜的参数
        OH_Any value;
        OH_EffectErrorCode errorCode = OH_EFilter_GetValue(filter, "brightness", &value)
        CHECK_AND_RETURN_LOG(errorCode == OH_EffectErrorCode::EFFECT_ERR_SUCCESS && value.dataType == OH_DataType::TYPE_FLOAT, result, "OH_EFilter_GetValue fail, errorCode = %{public}d" ,errorCode);
        float brightness = any.dataValue.floatValue;
        nlohmann::json values;
        value["brightness"] = brightness;
        // 生成键值对信息
        nlohmann::json root;
        root["name"] = "CustomBrightness";
        root["values"] = values;
        // 获取json字串，注意infoChar对象的最终释放
        std::string infoStr = root.dump();
        char *infoChar new char[infoStr.length() + 1]{0};
        std::strcpy(infoChar, infoStr.c_str());
        *info = infoChar;
        return true;
    }
    ```

    ```
    static OH_EFilter *Restore(const char *info)
    
    // 反序列化滤镜
    static OH_EFilter *Restore(const char *info)
    {
         // 解析滤镜名
         std::string infoStr = info;
         nlohmann::json jsonObject = nlohmann::json::parse(infoStr, nullptr, false);
         CHECK_AND_RETURN_LOG(jsonObject.find("name") != jsonObject.end() && jsonObject.at("name").is_string(), nullptr, "name not find");
         std::string name = jsonObject.at("name").get<std::string>();
         // 示例代码: 创建EFilter对象
         OH_EFilter *filter = OH_EFilter_Create(name.c_str());
         CHECK_AND_RETURN_LOG(filter != nullptr, result, "OH_EFilter_Create fail!");
         // 解析滤镜参数
         CHECK_AND_RETURN_LOG(jsonObject.find("values") != jsonObject.end() && jsonObject.at("values").is_object(), nullptr, "values not find");
          const nlohmann::json values= *(jsonObject.find("values"));
          CHECK_AND_RETURN_LOG(values.find("brightness") != values.end() && values.at("brightness").is_number(), nullptr, "brightness not find");
          
          //设置滤镜参数
          float brightness = values.at("brightness").get<float>();
          OH_Any value{
              .dataType = OH_DataType::TYPE_FLOAT,
              .dataValue.floatValue = brightness
          };
         
          OH_EffectErrorCode errorCode = OH_EFilter_SetValue(filter, "brightness", &value);
          CHECK_AND_RETURN_LOG(errorCode == OH_EffectErrorCode::EFFECT_ERR_SUCCESS, result, "OH_EFilter_SetValue fail, errorCode = %{public}d" ,errorCode);
          return filter;
    }
    ```

