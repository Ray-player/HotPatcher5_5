
#include "PakLoadRtSubsystem.h"
#include "IPlatformFilePak.h"
#include "Engine/StaticMeshActor.h"

void UPakLoadRtSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPakLoadRtSubsystem::Deinitialize()
{
	// 卸载所有挂载的Pak
	for (const auto& MountMes : MountMessages)
	{
		UnMountPakAsset(MountMes.OriPakPath);
	}
	MountMessages.Empty();
	Super::Deinitialize();
}

FPakPlatformFile* UPakLoadRtSubsystem::GetPakPlatform()
{
	if (!PakPlatformFile)
	{
		/*
			Packaged shipping builds will have a PakFile platform.
			For other build types a new pak platform file will be created.
		*/
		if (IPlatformFile *CurrentPlatformFile = FPlatformFileManager::Get().FindPlatformFile(TEXT("PakFile")))
		{
			//FLogHelper::Log(LL_VERBOSE, TEXT("Found PakPlatformFile"));
			UE_LOG(LogPakFile, Log, TEXT("Found PakPlatformFile"));
			PakPlatformFile = MakeShareable(static_cast<FPakPlatformFile*>(CurrentPlatformFile));
		}
		else
		{
			PakPlatformFile = MakeShareable(new FPakPlatformFile());

			ensure(PakPlatformFile != nullptr);

			IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

#if WITH_EDITOR
			// Keep the original platform file for non packaged builds.
			OriginalPlatformFile = &PlatformFile;
#endif

			if (PakPlatformFile->Initialize(&PlatformFile, TEXT("")))
			{
				FPlatformFileManager::Get().SetPlatformFile(*PakPlatformFile);
			}
			else
			{
				UE_LOG(LogPakFile, Log, TEXT("Failed to initialize PakPlatformFile"));
			}
		}
	}

	ensure(PakPlatformFile != nullptr);
	return PakPlatformFile.Get();
}

FMountMes UPakLoadRtSubsystem::GetMountedMessage(const FString& PakName)
{
	for (const auto& MountMes : MountMessages)
	{
		if (MountMes.OriPakPath.Contains(PakName))
		{
			return MountMes;
		}
	}
	return FMountMes();
}

TArray<FString> UPakLoadRtSubsystem::GetAllMountAssets()
{
	TArray<FString> AssetNames;
	for (const auto& MountMes : MountMessages)
	{
		AssetNames.Append(MountMes.PakAssets);
	}
	return AssetNames;
}

EPakResult UPakLoadRtSubsystem::MountPakAsset(const FString& InPakPath, bool bIsPluginPak)
{
	if (!GetMountedMessage(InPakPath).IsEmpty())
	{
		UE_LOG(LogPakFile, Warning, TEXT("Pak path already mounted: %s"), *InPakPath);
		return EPakResult::TrueOut;
	}
	IPlatformFile* OldPlatform = &FPlatformFileManager::Get().GetPlatformFile();

	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*InPakPath))
	{
		UE_LOG(LogPakFile, Warning, TEXT("Pak path not exists: %s"), *InPakPath);
		return EPakResult::FalseOut;
	}

	// 第三个参数是表示Pak是否加密
	const TRefCountPtr<FPakFile> TmpPak = new FPakFile(OldPlatform, *InPakPath, false);

	const FString OldPakMountPath = TmpPak->GetMountPoint();
	int32 FindPos;
	if (bIsPluginPak)
	{
		FindPos = OldPakMountPath.Find("Plugins/");
	}
	else
	{
		FindPos = OldPakMountPath.Find("Content/");
	}

	//这一步是为了将不同来源的资产的目前前缀给去掉，生成新的当前项目下的挂载点
	FString NewMountPath = OldPakMountPath.RightChop(FindPos);
	NewMountPath = FPaths::Combine(FPaths::ProjectDir(), NewMountPath);

	TmpPak->SetMountPoint(*NewMountPath);
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		UE_LOG(LogPakFile, Warning, TEXT("PIE模式下不能挂载Pak文件"));
		return EPakResult::FalseOut;
	}
	// PIE模式下不要执行Mount操作，不然大概率会跟你的现有资产冲突
	if (GetPakPlatform()->Mount(*InPakPath, 1, *NewMountPath))
	{
		TArray<FString> FoundFilenames;
		TmpPak->FindPrunedFilesAtPath( *TmpPak->GetMountPoint(),FoundFilenames, true, false, true);

		TArray<FString> ParsedAssets;
		for (const auto& Asset : FoundFilenames)
		{
			if (Asset.EndsWith(TEXT(".uasset")))
			{
				FString NewFileName = ConvertPakFile(Asset, bIsPluginPak);// 根据UE的规则，需要将原始路径转换下
				NewFileName.RemoveFromEnd(TEXT(".uasset"));
				ParsedAssets.Add(NewFileName);
			}
		}
		// 添加挂载信息
		MountMessages.Add(FMountMes(InPakPath, NewMountPath, ParsedAssets));
		return EPakResult::TrueOut;
	}
	return EPakResult::FalseOut;
}

bool UPakLoadRtSubsystem::UnMountPakAsset(const FString& PakPath)
{
	FPakPlatformFile* PakFileMgr = (FPakPlatformFile*)FPlatformFileManager::Get().GetPlatformFile(FPakPlatformFile::GetTypeName());
	if (!PakFileMgr)
	{
		UE_LOG(LogPakFile, Log, TEXT("GetPlatformFile(TEXT(\"PakFile\") is NULL"));
		return false;
	}

	if (!FPaths::FileExists(PakPath))
		return false;
	return PakFileMgr->Unmount(*PakPath);
}

UObject* UPakLoadRtSubsystem::LoadAssetAsObject(const FString& AssetName)
{
	return LoadUObjectFromPak<UObject>(AssetName);
}

UStaticMesh* UPakLoadRtSubsystem::LoadAssetAsStaticMesh(const FString& AssetName)
{
		return LoadUObjectFromPak<UStaticMesh>(AssetName);
}

FMountMes* UPakLoadRtSubsystem::GetMountedMesFromAssetName(const FString& AssetName)
{
	for (auto& MountMes : MountMessages)
	{
		if (MountMes.HasAsset(AssetName))
		{
			return &MountMes;
		}
	}
	return nullptr;
}

FString UPakLoadRtSubsystem::ConvertPakFile(const FString& InFileName, bool bIsPluginPak)
{
	FString NewFileName = InFileName;

	// 主要就是将原始目录变成平时进行资产引用的路径
	if (bIsPluginPak)
	{
		const FString PluginsPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins"));
		NewFileName.ReplaceInline(*PluginsPath, TEXT(""));
		NewFileName.ReplaceInline(TEXT("/Content"), TEXT(""));
	}
	else
	{
		const FString PathDir = FPaths::ProjectContentDir();
		NewFileName.ReplaceInline(*PathDir, TEXT("/Game/"));
	}

	return NewFileName;
}

void UPakLoadRtSubsystem::RegisterLevel(const FString& LevelName) const
{
}


