# HotPatcher5_5

## 概述

HotPatcher5_5 插件版本是基于 HotPatcher v82.0 版本进行修改生成的，主要针对性能优化和代码适配UE5.5版本进行了改进。
原版插件链接：[[Release v82.0 · hxhb/HotPatcher](https://github.com/hxhb/HotPatcher/releases/tag/v82.0)]

## 主要修改内容

### 1. 禁用更新检查功能

**修改文件：** `HotPatcherEditor.Build.cs`

**修改内容：**
将插件的自动更新检查功能禁用，减少不必要的网络请求和性能开销。

```cs
// HotPatcher5_2
PublicDefinitions.AddRange(new string[]
{
    "ENABLE_UPDATER_CHECK=1"
});

// HotPatcher5_5
PublicDefinitions.AddRange(new string[]
{
    "ENABLE_UPDATER_CHECK=0"
});
```

### 2. 禁用运行时自动加载着色器库

**修改文件：** `HotPatcherRuntime.Build.cs`

**修改内容：**
禁用了运行时自动加载 HotPatcher 相关着色器库的功能，改为手动加载方式，防止运行时此插件因着色器加载错误而崩溃。

```cs
// HotPatcher5_2
AddPublicDefinitions("AUTOLOAD_SHADERLIB_AT_RUNTIME", true);

// HotPatcher5_5
AddPublicDefinitions("AUTOLOAD_SHADERLIB_AT_RUNTIME", false);
```

### 3. 修复 Lambda 捕获问题

**修改文件：** `HotPatcherEditor.cpp`

**修改内容：**
修复了多处 Lambda 表达式中的捕获问题，将值捕获改为引用捕获或显式捕获，解决了潜在的变量生命周期问题，提高了代码安全性。

**修改点：**

- `PluginButtonClicked` 方法中，Lambda 捕获 `Context` 改为显式引用捕获
- `HandlePickingModeContextMenu` 方法中，Lambda 捕获 `Context` 改为显式引用捕获
- 多处菜单创建的 Lambda 表达式中，捕获方式优化

### 4. 增加Runtime加载Pak包模块

增加 `PakLoadRt` 模块用于运行时挂载&加载任意路径下的Pak资源包，该功能通过游戏实例子系统 `PakLoadRtSubsystem` 实现，并提供一系列方法给蓝图调用，对应Content目录下有示例UMG蓝图 `BP_LoadPak`供参考。
**主要函数：**

```C++
        //挂载Pak文件
	UFUNCTION(BlueprintPure, Category = "RtPakLoad")
	FMountMes GetMountedMessage(const FString& PakName);
	//获取所有挂载的资产
	UFUNCTION(BlueprintPure, Category = "RtPakLoad")
	TArray<FString> GetAllMountAssets();
	//挂载Pak文件资产
	UFUNCTION(BlueprintCallable, Category="RtPakLoad",meta = (ExpandEnumAsExecs = "ReturnValue"))
	EPakResult MountPakAsset(const FString& InPakPath,bool bIsPluginPak=false);
	//卸载挂载的资产
	UFUNCTION(BlueprintCallable, Category = "RtPakLoad")
	bool UnMountPakAsset(const FString& PakPath);

	//从Pak文件中加载资产
	UFUNCTION(BlueprintPure, Category="RtPakLoad")
	UObject* LoadAssetAsObject(const FString &AssetName);
	//从Pak文件中加载静态网格资产
	UFUNCTION(BlueprintPure, Category="RtPakLoad")
	UStaticMesh* LoadAssetAsStaticMesh(const FString &AssetName);
```

### 5. 源码API引用改进

**修改内容：**

1. 函数/宏/结构体等变更：

   ```C++
   //FlibReflectionHelper.cpp 26行与36行方法变更
   //原版
   Property->ExportTextItem(Value,Property->ContainerPtrToValuePtr<uint8>(Object),nullptr,Object,0);
   Property->ImportText(*Text,Property->ContainerPtrToValuePtr<uint8>(Object),0,Object);
   //5.5引擎版本
   Property->ExportTextItem_InContainer(Value,Property->ContainerPtrToValuePtr<uint8>(Object),nullptr,Object,0);
   Property->ImportText_InContainer(*Text,Property->ContainerPtrToValuePtr<uint8>(Object),Object,0);

    //FlibHotPatcherCoreHelper.cpp 310行宏中结构体变更
    //原版
    DECL_HACK_PRIVATE_DATA(UCookOnTheFlyServer, TUniquePtr<class FSandboxPlatformFile>, SandboxFile)
    //5.5引擎版本
    DECL_HACK_PRIVATE_DATA(UCookOnTheFlyServer, TUniquePtr<class UE::Cook::FCookSandbox>, SandboxFile)

    //FlibHotPatcherCoreHelper.cpp 641行枚举变更
    //原版
    FArchiveCookContext ArchiveCookContext(Package, FArchiveCookContext::ECookType::ECookByTheBook, FArchiveCookContext::ECookingDLC::ECookingDLCNo);
    //5.5引擎版本
    FArchiveCookContext ArchiveCookContext(Package, FArchiveCookContext::ECookType::ByTheBook, FArchiveCookContext::ECookingDLC::No);

    //FlibHotPatcherCoreHelper.cpp 1611行，函数传入参数变更：
    FSoftObjectPath SoftObjectPath(AssetPackagePath);
    if (State.GetAssetByObjectPath(SoftObjectPath))
   ```
2. 构造函数变更：

   ```C++
   //FlibHotPatcherCoreHelper.cpp 343行需要构造共享指针后创建FZenStoreWriter
    TSharedRef<FZenCookArtifactReader> CookRead= MakeShared<FZenCookArtifactReader>(ResolvedProjectPath, ResolvedMetadataPath, TargetPlatform);
    PackageWriter = new FZenStoreWriter(ResolvedProjectPath, ResolvedMetadataPath, TargetPlatform, CookRead);

    //349行需要实现FHotPatcherPackageWriter类的虚函数UpdatePackageModificationStatus
    void FHotPatcherPackageWriter::UpdatePackageModificationStatus(FName PackageName, bool bIterativelyUnmodified,bool& bInOutShouldIterativelySkip)
    {
    // 默认实现：如果包没有被迭代修改，则可以跳过
        if (bIterativelyUnmodified)
        {
    	    bInOutShouldIterativelySkip = true;
        }
    }
   ```
3. 头文件缺少
   `FlibHotPatcherCoreHelper.cpp` 添加 `AssetCompilingManager.h`
   `MissionNotificationProxy.cpp` 添加 `HotPatcherEditor.h`

## 影响分析

### 性能影响

- **启动速度提升**：禁用自动更新检查和运行时着色器加载，减少了插件启动时的开销
- **运行时内存占用降低**：减少了不必要的资源加载
- **网络请求减少**：不再自动检查更新，减少了网络流量

### 功能影响

- **更新检查功能禁用**：用户需要手动检查插件更新
- **着色器库需手动加载**：需要在合适的时机手动调用着色器库加载方法
- **核心功能保持不变**：补丁创建、资源打包等核心功能未受影响

## 升级建议

1. **对于开发者**：

   - 如果需要使用更新检查功能，可以在 `HotPatcherEditor.Build.cs` 中将 `ENABLE_UPDATER_CHECK` 改回 1
   - 如果需要运行时自动加载着色器库，可以在 `HotPatcherRuntime.Build.cs` 中将 `AUTOLOAD_SHADERLIB_AT_RUNTIME` 改回 true
2. **对于普通用户**：

   - 建议直接使用 5_5 版本，性能更优
   - 如果遇到着色器相关问题，可以手动调用 `UFlibPakHelper::LoadHotPatcherAllShaderLibrarys()` 方法
