// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ETargetPlatform.h"
#include "Widgets/SCompoundWidget.h"
#include "Interfaces/ITargetPlatform.h"

class FHotPatcherEditorModule;

class SCookAndPakSettingsWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCookAndPakSettingsWindow) {}
		SLATE_ARGUMENT(ETargetPlatform, TargetPlatform)
		SLATE_ARGUMENT(FHotPatcherEditorModule*, EditorModule)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

private:
	// UI元素
	TSharedPtr<SCheckBox> bNoDependenciesCheckBox;
	TSharedPtr<SEditableTextBox> PakNameTextBox;
	TSharedPtr<SEditableTextBox> OutputPathTextBox;

	// 数据
	ETargetPlatform TargetPlatform;
	FHotPatcherEditorModule* EditorModule;
	FString DefaultOutputPath;

	// 回调函数
	FReply OnBrowseOutputPath();
	FReply OnOKButtonClicked();
	FReply OnCancelButtonClicked();

	// 获取设置值
	bool IsNoDependenciesChecked() const;
	FString GetPakName() const;
	FString GetOutputPath() const;
};
