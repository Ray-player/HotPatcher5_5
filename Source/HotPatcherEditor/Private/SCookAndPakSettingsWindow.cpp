// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SCookAndPakSettingsWindow.h"
#include "HotPatcherEditor.h"
#include "FlibHotPatcherEditorHelper.h"
#include "FlibHotPatcherCoreHelper.h"
#include "Kismet/KismetTextLibrary.h"

#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Misc/EngineVersionComparison.h"
#include "Widgets/Text/STextBlock.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"

#if !UE_VERSION_OLDER_THAN(5,1,0)
	typedef FAppStyle FEditorStyle;
#endif

#define LOCTEXT_NAMESPACE "SCookAndPakSettingsWindow"

void SCookAndPakSettingsWindow::Construct(const FArguments& InArgs)
{
	TargetPlatform = InArgs._TargetPlatform;
	EditorModule = InArgs._EditorModule;
	DefaultOutputPath = FPaths::ProjectDir();//UFlibHotPatcherCoreHelper::GetDefaultHotPatcherOutputDir();

	ChildSlot
	[ 
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(10.0f)
		[ 
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[ 
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("Title", "Cook and Pak Settings for {0}"), 
						UKismetTextLibrary::Conv_StringToText(THotPatcherTemplateHelper::GetEnumNameByValue(TargetPlatform))))
					.Font(FEditorStyle::GetFontStyle("LargeText"))
				]

			// 第一行：NoDependencies复选框
			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[ 
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
						.FillWidth(0.3f)
						.VAlign(VAlign_Center)
						[ 
							SNew(STextBlock)
							.Text(LOCTEXT("NoDependenciesLabel", "No Dependencies:"))
						]
					+ SHorizontalBox::Slot()
						.FillWidth(0.7f)
						[ 
							SAssignNew(bNoDependenciesCheckBox, SCheckBox)
							.HAlign(HAlign_Left)
							.OnCheckStateChanged_Lambda([](ECheckBoxState NewState) {})
							.ToolTipText(LOCTEXT("NoDependenciesTooltip", "If checked, the pak will be created without analyzing dependencies"))
							[ 
								SNew(STextBlock)
								.Text(LOCTEXT("NoDependenciesText", "Generate pak without analyzing dependencies"))
							]
						]
				]

			// 第二行：Pak包名称
			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[ 
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
						.FillWidth(0.3f)
						.VAlign(VAlign_Center)
						[ 
							SNew(STextBlock)
							.Text(LOCTEXT("PakNameLabel", "Pak Name:"))
						]
					+ SHorizontalBox::Slot()
						.FillWidth(0.7f)
						[ 
							SAssignNew(PakNameTextBox, SEditableTextBox)
							.HintText(LOCTEXT("PakNameHint", "Leave empty for default naming"))
							.ToolTipText(LOCTEXT("PakNameTooltip", "Enter custom pak name, leave empty for default"))
						]
				]

			// 第三行：输出路径
			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[ 
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
						.FillWidth(0.3f)
						.VAlign(VAlign_Center)
						[ 
							SNew(STextBlock)
							.Text(LOCTEXT("OutputPathLabel", "Output Path:"))
						]
					+ SHorizontalBox::Slot()
						.FillWidth(0.6f)
						[ 
							SAssignNew(OutputPathTextBox, SEditableTextBox)
							.Text(FText::FromString(DefaultOutputPath))
							.HintText(LOCTEXT("OutputPathHint", "Leave empty for default path"))
							.ToolTipText(LOCTEXT("OutputPathTooltip", "Enter custom output path, leave empty for default"))
						]
					+ SHorizontalBox::Slot()
						.AutoWidth()
						[ 
							SNew(SButton)
							.Text(LOCTEXT("BrowseButton", "Browse"))
							.OnClicked(this, &SCookAndPakSettingsWindow::OnBrowseOutputPath)
						]
				]

			// 按钮行
			+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[ 
					SNew(SUniformGridPanel)
					.SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding"))
					.MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
					.MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))

					+ SUniformGridPanel::Slot(0, 0)
					[ 
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("OKButton", "OK"))
						.OnClicked(this, &SCookAndPakSettingsWindow::OnOKButtonClicked)
					]

					+ SUniformGridPanel::Slot(1, 0)
					[ 
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("CancelButton", "Cancel"))
						.OnClicked(this, &SCookAndPakSettingsWindow::OnCancelButtonClicked)
					]
				]
		]
	];
}

FReply SCookAndPakSettingsWindow::OnBrowseOutputPath()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
		FString OutFolderName;

		if (DesktopPlatform->OpenDirectoryDialog(ParentWindowHandle, 
			LOCTEXT("BrowseOutputPathTitle", "Select Output Path").ToString(), 
			OutputPathTextBox->GetText().ToString(), 
			OutFolderName))
		{
			OutputPathTextBox->SetText(FText::FromString(OutFolderName));
		}
	}
	return FReply::Handled();
}

FReply SCookAndPakSettingsWindow::OnOKButtonClicked()
{
	// 执行打包逻辑
	bool bNoDependencies = IsNoDependenciesChecked();
	FString PakName = GetPakName();
	FString OutputPath = GetOutputPath();

	// 调用现有的打包函数
	EditorModule->OnCookAndPakPlatform(TargetPlatform, !bNoDependencies);

	// 关闭窗口
	FSlateApplication::Get().RequestDestroyWindow(FSlateApplication::Get().FindWidgetWindow(SharedThis(this)).ToSharedRef());
	return FReply::Handled();
}

FReply SCookAndPakSettingsWindow::OnCancelButtonClicked()
{
	// 关闭窗口
	FSlateApplication::Get().RequestDestroyWindow(FSlateApplication::Get().FindWidgetWindow(SharedThis(this)).ToSharedRef());
	return FReply::Handled();
}

bool SCookAndPakSettingsWindow::IsNoDependenciesChecked() const
{
	//return bNoDependenciesCheckBox->GetCheckedState() == ECheckBoxState::Checked;
	return bNoDependenciesCheckBox->IsChecked();
}

FString SCookAndPakSettingsWindow::GetPakName() const
{
	return PakNameTextBox->GetText().ToString();
}

FString SCookAndPakSettingsWindow::GetOutputPath() const
{
	FString Path = OutputPathTextBox->GetText().ToString();
	return Path.IsEmpty() ? DefaultOutputPath : Path;
}

#undef LOCTEXT_NAMESPACE
