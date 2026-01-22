#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PakLoadRtSubsystem.generated.h"

class FPakPlatformFile;

UENUM(BlueprintType)
enum class EPakResult : uint8
{
	TrueOut,
	FalseOut
};

USTRUCT(BlueprintType)
struct FMountMes
{
	GENERATED_BODY()
public:
	bool IsEmpty()
	{
		return OriPakPath.IsEmpty() && NowMountPoint.IsEmpty() && PakAssets.IsEmpty();
	}
	bool HasAsset(const FString& AssetName)
	{
		return PakAssets.Contains(AssetName);
	}
	FMountMes()
	{
		OriPakPath = TEXT("");
		NowMountPoint = TEXT("");
		PakAssets.Empty();
	}
	FMountMes(FString InOriPakPath, FString InNowMountPoint, const TArray<FString>& InPakAssets)
	{
		OriPakPath = InOriPakPath;
		NowMountPoint = InNowMountPoint;
		PakAssets = InPakAssets;
	}
	UPROPERTY(BlueprintReadWrite, Category = "MountMessage")
	FString OriPakPath;
	UPROPERTY(BlueprintReadWrite, Category = "MountMessage")
	FString NowMountPoint;
	UPROPERTY(BlueprintReadWrite, Category = "MountMessage")
	TArray<FString> PakAssets;
};

/**
 * 运行时加载Pak包,子系统
 */
UCLASS()
class PAKLOADRT_API UPakLoadRtSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FPakPlatformFile *GetPakPlatform();
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

	
	FMountMes* GetMountedMesFromAssetName(const FString& AssetName);
	
	template<class T>
	T *LoadUObjectFromPak(const FString &Filename)
	{
		//const FString Name = T::StaticClass()->GetName() + TEXT("'") + Filename + TEXT(".") + FPackageName::GetShortName(Filename) + TEXT("'");
		return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *Filename));
	}
private:
	TSharedPtr<FPakPlatformFile> PakPlatformFile;
	TArray<FMountMes> MountMessages;
	FString ConvertPakFile(const FString& InFileName, bool bIsPluginPak);
	void RegisterLevel(const FString& LevelName) const;
#if WITH_EDITOR
	IPlatformFile* OriginalPlatformFile = nullptr;
#endif
};
