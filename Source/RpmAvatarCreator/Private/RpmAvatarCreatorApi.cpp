// Copyright © 2023++ Ready Player Me


#include "RpmAvatarCreatorApi.h"

#include "RpmPartnerAssetLoader.h"
#include "RpmAuthManager.h"
#include "RpmAvatarRequestHandler.h"
#include "Requests/RequestFactory.h"
#include "Utils/PayloadUpdater.h"

URpmAvatarCreatorApi::URpmAvatarCreatorApi()
	: TargetSkeleton(nullptr)
{
	AuthManager = MakeShared<FRpmAuthManager>();
	AssetLoader = NewObject<URpmPartnerAssetLoader>();
	AvatarRequestHandler = NewObject<URpmAvatarRequestHandler>();
}

void URpmAvatarCreatorApi::Authenticate(const FAuthenticationCompleted& Completed, const FAvatarCreatorFailed& Failed)
{
	AvatarProperties = FPayloadUpdater::MakeAvatarProperties();
	OnAuthenticationCompleted = Completed;
	OnAvatarCreatorFailed = Failed;
	RequestFactory = MakeShared<FRequestFactory>(PartnerDomain);
	AuthManager->GetCompleteCallback().BindUObject(this, &URpmAvatarCreatorApi::OnAuthComplete);
	AuthManager->AuthAnonymous(RequestFactory);

	AvatarRequestHandler->Initialize(RequestFactory, OnPreviewDownloaded);
}

void URpmAvatarCreatorApi::OnAuthComplete(bool bSuccess)
{
	if (!bSuccess)
	{
		(void)OnAvatarCreatorFailed.ExecuteIfBound(ERpmAvatarCreatorError::AuthenticationFailure);
		return;
	}
	const FRpmUserSession Session = AuthManager->GetUserSession();
	RequestFactory->SetAuthToken(Session.Token);
	(void)OnAuthenticationCompleted.ExecuteIfBound();
}

void URpmAvatarCreatorApi::PrepareEditor(const FAvatarEditorReady& EditorReady, const FAvatarCreatorFailed& Failed)
{
	if (!AssetLoader->AreAssetsReady())
	{
		AssetLoader->GetPartnerAssetsDownloadCallback().BindUObject(this, &URpmAvatarCreatorApi::AssetsDownloaded, EditorReady, Failed);
		AssetLoader->DownloadAssets(RequestFactory);
	}

	AvatarRequestHandler->GetAvatarCreatedCallback().BindUObject(this, &URpmAvatarCreatorApi::AvatarCreated, EditorReady, Failed);
	AvatarRequestHandler->CreateAvatar(AvatarProperties, TargetSkeleton);
}

void URpmAvatarCreatorApi::AssetsDownloaded(bool bSuccess, FAvatarEditorReady EditorReady, FAvatarCreatorFailed Failed)
{
	if (!bSuccess)
	{
		(void)Failed.ExecuteIfBound(ERpmAvatarCreatorError::AssetDownloadFailure);
		return;
	}
	if (AvatarRequestHandler->Mesh)
	{
		(void)EditorReady.ExecuteIfBound();
		(void)AvatarRequestHandler->OnPreviewDownloaded.ExecuteIfBound(AvatarRequestHandler->Mesh);
	}
}

void URpmAvatarCreatorApi::AvatarCreated(bool bSuccess, FAvatarEditorReady EditorReady, FAvatarCreatorFailed Failed)
{
	if (!bSuccess)
	{
		(void)Failed.ExecuteIfBound(ERpmAvatarCreatorError::AvatarPreviewFailure);
		return;
	}
	if (AssetLoader->AreAssetsReady())
	{
		(void)EditorReady.ExecuteIfBound();
		(void)AvatarRequestHandler->OnPreviewDownloaded.ExecuteIfBound(AvatarRequestHandler->Mesh);
	}
}

void URpmAvatarCreatorApi::UpdateAvatar(ERpmPartnerAssetType AssetType, int64 AssetId)
{
	AvatarRequestHandler->UpdateAvatar(AssetType, AssetId);
}

void URpmAvatarCreatorApi::SaveAvatar(const FAvatarSaveCompleted& AvatarSaveCompleted, const FAvatarCreatorFailed& Failed)
{
	AvatarRequestHandler->SaveAvatar(AvatarSaveCompleted, Failed);
}

FRpmAvatarProperties URpmAvatarCreatorApi::GetAvatarProperties() const
{
	return AvatarRequestHandler->GetAvatarProperties();
}

const TArray<FRpmPartnerAsset>& URpmAvatarCreatorApi::GetPartnerAssets() const
{
	return AssetLoader->Assets;
}

void URpmAvatarCreatorApi::BeginDestroy()
{
	//TODO: Cancel Load
	Super::BeginDestroy();
}
