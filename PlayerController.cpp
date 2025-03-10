#include "PlayerController.h"
#include "Input.h"
#include "Transform.h"
#include "ObjectBase.h"
#include "Rigidbody.h"
#include "AABBCollider.h"
#include "XInput.h"
#include "Primitive.h"
#include "Vector3.h"
#include "Float3.h"
#include "CameraDebug.h"
#include "ObjectManager.h"
#include "Arrow.h"
#include <math.h>
#include "CameraPlayer.h"
#include "Life.h"	//
#include "LifeNumber.h"//
#include "Zanki.h" //
#include "SceneManager.h" // 
#include "clicAtk.h"
#include "AtkGauge.h"
#include "FadeManager.h"
#include "XInput.h"
#include "SpesialGauge.h"

void PlayerController::Start()
{
	// 当たり判定をとるタグ名を設定
	GetOwner()->GetComponent<AABBCollider>()->SetTouchOBB(TagName::Ground);
	GetOwner()->GetComponent<AABBCollider>()->SetTouchOBB(TagName::Wall);
	
	// 設定した残機の数でUIを初期化
	if (m_Zanki == 8) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(1);
	if (m_Zanki == 7) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(2);
	if (m_Zanki == 6) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(3);
	if (m_Zanki == 5) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(4);
	if (m_Zanki == 4) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(5);
	if (m_Zanki == 3) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(6);
	if (m_Zanki == 2) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(7);
	if (m_Zanki == 1) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(8);
	if (m_Zanki == 0) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(9);

	//通常攻撃のゲージを初期化
	ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(0);

}

void PlayerController::Update()
{
	// カメラがPlayerカメラではない時は更新しない
	if (!ObjectManager::FindObjectWithTag(TagName::MainCamera)->GetComponent<CameraPlayer>()) return;
	
	// 座標を保存する
	m_prevPos = GetOwner()->GetComponent<Transform>()->GetPosition();

	//--- 移動（カメラの向きに応じて移動方向を決める）
	// メインカメラの座標と注視点を取得する
	DirectX::XMFLOAT3 camPos = ObjectManager::FindObjectWithTag(TagName::MainCamera)->GetComponent<Transform>()->GetPosition();
	DirectX::XMFLOAT3 camLook = ObjectManager::FindObjectWithTag(TagName::MainCamera)->GetComponent<CameraPlayer>()->GetLookPoint();
	// Y軸でのプレイヤーの移動は要らないため、0.0fに設定
	camPos.y = 0.0f;
	camLook.y = 0.0f;
	// XMVECTORに変換
	DirectX::XMVECTOR vCamPos = DirectX::XMLoadFloat3(&camPos);
	DirectX::XMVECTOR vCamLook = DirectX::XMLoadFloat3(&camLook);
	// 座標から注視点へ向くベクトルを算出(正面)
	DirectX::XMVECTOR vFront;
	vFront = DirectX::XMVectorSubtract(vCamLook, vCamPos);
	vFront = DirectX::XMVector3Normalize(vFront);
	// 正面方向に対して、Y軸を90°回転させた横向きのベクトルを算出
	DirectX::XMMATRIX matRotSide = DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(90.0f));
	DirectX::XMVECTOR vSide = DirectX::XMVector3TransformCoord(vFront, matRotSide);

	DirectX::XMVECTOR vMove = DirectX::XMVectorZero();
	// 斜め移動も可
	if (IsKeyPress('W') || XInput::GetJoyPOVButton(0, CURSOR_BUTTON_TYPE::POV_UP)   ) vMove = DirectX::XMVectorAdd(vMove, vFront);
	if (IsKeyPress('S') || XInput::GetJoyPOVButton(0, CURSOR_BUTTON_TYPE::POV_DOWN) ) vMove = DirectX::XMVectorAdd(vMove, DirectX::XMVectorScale(vFront, -1.0f));
	if (IsKeyPress('A') || XInput::GetJoyPOVButton(0, CURSOR_BUTTON_TYPE::POV_LEFT) ) vMove = DirectX::XMVectorAdd(vMove, DirectX::XMVectorScale(vSide, -1.0f));
	if (IsKeyPress('D') || XInput::GetJoyPOVButton(0, CURSOR_BUTTON_TYPE::POV_RIGHT)) vMove = DirectX::XMVectorAdd(vMove, vSide);
	// 斜め移動のときに移動量が多くなってしまうため、正規化する
	vMove = DirectX::XMVector3Normalize(vMove);
	vMove = DirectX::XMVectorScale(vMove, 0.3f);

	DirectX::XMFLOAT3 move;
	DirectX::XMStoreFloat3(&move, vMove);
	// ノックバック中であれば
	if (m_bKnockBackFlg) {
		m_KnockBackCount--;
		if (m_KnockBackCount < 0) {
			m_bKnockBackFlg = false;
			m_KnockBackCount = 20;
		}
	}
	else 
	{
		// 移動量を加える
		GetOwner()->GetComponent<Transform>()->SetPosition({
			GetOwner()->GetComponent<Transform>()->GetPosition().x + move.x,
			GetOwner()->GetComponent<Transform>()->GetPosition().y + move.y,
			GetOwner()->GetComponent<Transform>()->GetPosition().z + move.z
			});
	}
	// 移動した場合、移動した方向に回転する
	if (move.x != 0.0f || move.y != 0.0f || move.z != 0.0f) {
		float radY = 0.0f;
		// Z方向へのベクトル(モデルの正面方向のベクトル)
		DirectX::XMFLOAT3 zVector = { 0.0f, 0.0f, 1.0f };
		// 内積とベクトルの長さを使ってcosθを求める
		DirectX::XMStoreFloat(&radY, DirectX::XMVector3Dot(DirectX::XMVector3Normalize(vMove), DirectX::XMLoadFloat3(&zVector)));
		// 内積から角度を求める
		radY = ::acos(radY);
		// ラジアン角からおなじみの角度に変更
		radY = DirectX::XMConvertToDegrees(radY);
		// 回転が右回転か左回転かを判別するために、外積で求める
		// 求めた外積のY成分がプラスだったら左回り。
		// 求めた外積のY成分がマイナスだったら右回り。
		DirectX::XMFLOAT3 rotateDirection;
		DirectX::XMStoreFloat3(&rotateDirection, DirectX::XMVector3Cross(DirectX::XMVector3Normalize(vMove), DirectX::XMLoadFloat3(&zVector)));
		if (rotateDirection.y > 0) radY = 180.0f + (180.0f - radY);
		// 算出した角度を適用する
		GetOwner()->GetComponent<Transform>()->SetAngle({ 0.0f, radY, 0.0f });
	}

	
	//------------矢の処理--------------
	//通常の矢の処理
	IsNormalArrow();

	//スペシャル用の矢の処理
	if (m_EnableSpecial) IsSpecialArrow();
	
	//----------------------------------
	
	//--- 座標補正
	// 落下判定
	if (GetOwner()->GetComponent<Rigidbody>()) {
		// y座標が-9以下
		if (GetOwner()->GetComponent<Transform>()->GetPosition().y < -9.0f) {
			// 座標を補正
			GetOwner()->GetComponent<Transform>()->SetPosition({
				GetOwner()->GetComponent<Transform>()->GetPosition().x,
				0.0f,
				GetOwner()->GetComponent<Transform>()->GetPosition().z
				});
			// 加速度を補正
			GetOwner()->GetComponent<Rigidbody>()->SetAccele({ 0.0f, 0.0f, 0.0f });
		}
	}


	//UI関係の処理------
	if (m_LivesHighlighting)	//残機強調表示中
	{
		LivesHighlight();
	}

	//--------------------------------------
	// 1/18
	//      Update関数で体力テクスチャを変更
	//--------------------------------------
	// HP_MAX
	if (m_Life == 4)
	{
		ObjectManager::FindObjectWithName("UI.5")->GetComponent<Life>()->Swapframe(0);
		ObjectManager::FindObjectWithName("UI.5")->GetComponent<Life>()->Play();
		ObjectManager::FindObjectWithName("UI.11")->GetComponent<LifeNumber>()->Swapframe(0);
		ObjectManager::FindObjectWithName("UI.11")->GetComponent<LifeNumber>()->Play();
	}
	//HP_75%
	if (m_Life == 3)
	{
		ObjectManager::FindObjectWithName("UI.5")->GetComponent<Life>()->Swapframe(1);
		ObjectManager::FindObjectWithName("UI.5")->GetComponent<Life>()->Play();
		ObjectManager::FindObjectWithName("UI.11")->GetComponent<LifeNumber>()->Swapframe(1);
		ObjectManager::FindObjectWithName("UI.11")->GetComponent<LifeNumber>()->Play();
	}
	//HP_50%
	if (m_Life == 2)
	{
		ObjectManager::FindObjectWithName("UI.5")->GetComponent<Life>()->Swapframe(2);
		ObjectManager::FindObjectWithName("UI.5")->GetComponent<Life>()->Play();
		ObjectManager::FindObjectWithName("UI.11")->GetComponent<LifeNumber>()->Swapframe(2);
		ObjectManager::FindObjectWithName("UI.11")->GetComponent<LifeNumber>()->Play();
	}
	//HP_25%
	if (m_Life == 1)
	{
		ObjectManager::FindObjectWithName("UI.5")->GetComponent<Life>()->Swapframe(3);
		ObjectManager::FindObjectWithName("UI.5")->GetComponent<Life>()->Play();
		ObjectManager::FindObjectWithName("UI.11")->GetComponent<LifeNumber>()->Swapframe(3);
		ObjectManager::FindObjectWithName("UI.11")->GetComponent<LifeNumber>()->Play();
	}
	//HP_0%
	if (m_Life == 0)
	{
		ObjectManager::FindObjectWithName("UI.5")->GetComponent<Life>()->Swapframe(4);
		ObjectManager::FindObjectWithName("UI.5")->GetComponent<Life>()->Play();
	}
	//HP_0 -> MAX
	if (m_Life <= 0)
	{
		m_Life = 4;
		ObjectManager::FindObjectWithName("UI.5")->GetComponent<Life>()->Swapframe(0);
		ObjectManager::FindObjectWithName("UI.5")->GetComponent<Life>()->Play();
		m_Zanki--;
		if (m_Zanki == 8) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(1);
		if (m_Zanki == 7) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(2);
		if (m_Zanki == 6) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(3);
		if (m_Zanki == 5) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(4);
		if (m_Zanki == 4) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(5);
		if (m_Zanki == 3) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(6);
		if (m_Zanki == 2) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(7);
		if (m_Zanki == 1) ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(8);


		//残機UI減少ハイライト----------小栗
		m_LivesHighlighting = true;

		// 残機が0でシーン移動
		if (m_Zanki <= 0)
		{
			ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->Swapframe(9);
			//SceneManager::LoadScene(SceneName::SceneResult);
			FadeManager::CreateFadeOut(SceneName::SceneResult);
		}
	}
	//mc_nClearEnemyNumの数だけ倒すとクリア--------
	//if (m_nEnemyNum == mc_nClearEnemyNum)
	//{
	//	//SceneManager::LoadScene(SceneName::SceneGame01);
	//	FadeManager::CreateFadeOut(SceneName::SceneGame01);
	//}
}

void PlayerController::OnCollisionEnter(ObjectBase* object)
{
	// フィールドと当たったときの処理
	if (object->GetTag() == TagName::Ground) {
		// Y軸の加速度をゼロに
		this->GetOwner()->GetComponent<Rigidbody>()->SetAccele({
			this->GetOwner()->GetComponent<Rigidbody>()->GetAccele().x,
			0.0f,
			this->GetOwner()->GetComponent<Rigidbody>()->GetAccele().z
			});

		// --- めり込んだ位置からY方向に戻し距離だけオフセットする
		float offsetPosY =
			object->GetComponent<AABBCollider>()->GetPrimitive().p.y +
			object->GetComponent<AABBCollider>()->GetPrimitive().hl.y +
			this->GetOwner()->GetComponent<AABBCollider>()->GetPrimitive().hl.y;
		this->GetOwner()->GetComponent<Transform>()->SetPosition({
			this->GetOwner()->GetComponent<Transform>()->GetPosition().x,
			offsetPosY,
			this->GetOwner()->GetComponent<Transform>()->GetPosition().z
			});
	}

	// 壁と当たったときの処理
	if (object->GetTag() == TagName::Wall) {
		GetOwner()->GetComponent<Transform>()->SetPosition(m_prevPos);

		// 加速度を補正
		GetOwner()->GetComponent<Rigidbody>()->SetAccele({ 0.0f, 0.0f, 0.0f });
	}

	//-------------------------------------------------------------------------------
	// 12/22
	// 竹下　敵と当たった時の朱里
	// 1/24  タグネームがめちゃんこ増えてるのでその分当たり判定を追加
	//-------------------------------------------------------------------------------
	if (object->GetTag() == TagName::Enemy || 
		object->GetTag() == TagName::MiddleBoss||
		object->GetTag() == TagName::GenerateEnemy||
		object->GetTag() == TagName::GenerateStrEnemy||
		object->GetTag() == TagName::FinalBoss||
		object->GetTag() == TagName::FinalBigBoss||
		object->GetTag() == TagName::StrFinalBoss||
		object->GetTag() == TagName::ShockWave)
	{
		m_bLifeFlg = true;
		if (m_bLifeFlg)
		{
			m_FlgCount--;
			// ダメージ量を敵の種類によって分割
			if (object->GetTag() == TagName::Enemy)
				Reduce(1); // ライフが1減る
			if (object->GetTag() == TagName::MiddleBoss)
				Reduce(2); // ライフが2減る
			if (object->GetTag() == TagName::GenerateEnemy)
				Reduce(1); // ライフが1減る
			if (object->GetTag() == TagName::GenerateStrEnemy)
				Reduce(1); // ライフが1減る
			if (object->GetTag() == TagName::FinalBoss)
				Reduce(1); // ライフが1減
			if (object->GetTag() == TagName::FinalBigBoss)
				Reduce(1); // ライフが1減る
			if (object->GetTag() == TagName::StrFinalBoss)
				Reduce(1); // ライフが1減る
			if (object->GetTag() == TagName::ShockWave)
				Reduce(1); // ライフが1減る
			if (m_FlgCount <= 0)
			{
				//初期化処理
				m_bLifeFlg = false;
				m_FlgCount = 5.0f;
			}
		}	
		ObjectManager::FindObjectWithTag(TagName::MainCamera)->GetComponent<CameraPlayer>()->ScreenShake();
	}
}

void PlayerController::OnCollisionStay(ObjectBase* object)
{
	// フィールドと当たったときの処理
	if (object->GetTag() == TagName::Ground) {
		// Y軸の加速度をゼロに
		this->GetOwner()->GetComponent<Rigidbody>()->SetAccele({
			this->GetOwner()->GetComponent<Rigidbody>()->GetAccele().x,
			0.0f,
			this->GetOwner()->GetComponent<Rigidbody>()->GetAccele().z
			});

		// --- めり込んだ位置からY方向に戻し距離だけオフセットする
		float offsetPosY =
			object->GetComponent<AABBCollider>()->GetPrimitive().p.y +
			object->GetComponent<AABBCollider>()->GetPrimitive().hl.y +
			this->GetOwner()->GetComponent<AABBCollider>()->GetPrimitive().hl.y;
		this->GetOwner()->GetComponent<Transform>()->SetPosition({
			this->GetOwner()->GetComponent<Transform>()->GetPosition().x,
			offsetPosY,
			this->GetOwner()->GetComponent<Transform>()->GetPosition().z
			});
	}

	// 壁と当たったときの処理
	if (object->GetTag() == TagName::Wall) {
		GetOwner()->GetComponent<Transform>()->SetPosition(m_prevPos);
		// 加速度を補正
		GetOwner()->GetComponent<Rigidbody>()->SetAccele({ 0.0f, 0.0f, 0.0f });
	}
}

void PlayerController::OnCollisionExit(ObjectBase* object)
{
}


//================
//体力回復関数
//
//
// 引数：int add
//================
void PlayerController::AddLife(int add)
{
	// 体力が最大であれば処理しない
	if (m_Life == MAX_LIFE) return;

	m_Life += add;

	//体力が上限を超えないようにるする
	if (m_Life > MAX_LIFE) m_Life = MAX_LIFE;
}

//--------------------
// 竹下 Reduce関数で体力を減らす
void PlayerController::Reduce(int num)
{
	m_Life -= num;
}

//小栗大輝----------------------
void PlayerController::LivesHighlight()
{
	//終了したかどうか
	bool endflg = false;

	if(m_LivesIV.x <= 2.0f && m_LivesHalf == false)
	{
		m_LivesIV.x += 0.1f;
		m_LivesIV.y += 0.3f;
		ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->GetOwner()->GetComponent<SpriteRenderer>()->SetSize(DirectX::XMFLOAT2(m_LivesIV.x,m_LivesIV.y));
		ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->GetOwner()->GetComponent<SpriteRenderer>()->SetColor(DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
		if (m_LivesIV.x >= 2.0f)	m_LivesHalf = true;
	}
	if(m_LivesHalf)
	{
		m_LivesIV.x -= 0.1f;
		m_LivesIV.y -= 0.3f;
		ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->GetOwner()->GetComponent<SpriteRenderer>()->SetSize(DirectX::XMFLOAT2(m_LivesIV.x, m_LivesIV.y));
		//初期値と同じサイズに戻ったらフラグを折る
		if (m_LivesIV.x <= 1.0f)	endflg = true;
	}

	if (endflg)	//終了時元の色に戻してアニメーション中フラグを下げる
	{
		ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->GetOwner()->GetComponent<SpriteRenderer>()->SetColor(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));	//色を戻す
		ObjectManager::FindObjectWithName("UI.9")->GetComponent<Zanki>()->GetOwner()->GetComponent<SpriteRenderer>()->SetSize(DirectX::XMFLOAT2(1.0f, 1.0f));	//大きさを戻す
		m_LivesHighlighting = false;
		m_LivesHalf = false;
	}


}



//======================
//スペシャル用の矢
//======================
void PlayerController::IsSpecialArrow()
{
	// 矢の発射　単発うち
	if (IsKeyTrigger(VK_SPACE)
		|| XInput::GetJoyTrigger(0, BUTTON_TYPE::R)) {
		m_tic = 0.0f;	// 押し始めたら、0.0fに初期化

		// 変更用ポインタ
		std::shared_ptr<Transform> trans[2];

		//--- オブジェクト作成
		//   型　：Arrow
		//  名前 ：Arrow
		// タグ名：Arrow
		for (int i = 1; i < 3; i++)
		{
			m_haveArrow[i] = ObjectManager::CreateObject<Arrow>("Arrow", TagName::Arrow);
			// 今持っているArrowのTransformを取得
			trans[i - 1] = m_haveArrow[i]->GetComponent<Transform>();
			// 座標を自分のオブジェクト＋自分オブジェクトの法線（長さ１）横の位置に設定
			// 要約：右前
			trans[i - 1]->SetPosition({
				GetOwner()->GetComponent<Transform>()->GetPosition().x +
					GetOwner()->GetComponent<Transform>()->GetVectorRight().x +
					GetOwner()->GetComponent<Transform>()->GetVectorForword().x,
				GetOwner()->GetComponent<Transform>()->GetPosition().y +
					GetOwner()->GetComponent<Transform>()->GetVectorRight().y +
					GetOwner()->GetComponent<Transform>()->GetVectorForword().y,
				GetOwner()->GetComponent<Transform>()->GetPosition().z +
					GetOwner()->GetComponent<Transform>()->GetVectorRight().z +
					GetOwner()->GetComponent<Transform>()->GetVectorForword().z
				});
			// 角度を自分のオブジェクトの角度に設定
			trans[i - 1]->SetAngle({
				GetOwner()->GetComponent<Transform>()->GetAngle().x,
				GetOwner()->GetComponent<Transform>()->GetAngle().y + 0.0f,// 矢のモデルと矢を射出するモデルの正面が違う場合、ここで数値調整する。
				GetOwner()->GetComponent<Transform>()->GetAngle().z
				});
		}
	}

	//スペースを押している間
	if (IsKeyPress(VK_SPACE)
		|| XInput::GetJoyButton(0, BUTTON_TYPE::R)) {

		m_tic++;	// 押している間、カウントする
		// 変更用ポインタ
		std::shared_ptr<Transform> trans[2];
		std::shared_ptr<Rigidbody> rb[2];
		for (int i = 1; i < 3; i++)
		{
			// 今持っているArrowのTransformを取得
			trans[i - 1] = m_haveArrow[i]->GetComponent<Transform>();
			// 今持っているArrowのRigidbodyを取得
			rb[i - 1] = m_haveArrow[i]->GetComponent<Rigidbody>();
			// チャージ時間の割合を求める
			float ChargePer = m_tic > m_ChargeTime ? m_ChargeTime / m_ChargeTime : m_tic / m_ChargeTime;

			// 割合を逆にする
			ChargePer = 1 - ChargePer;
			// 座標を自分のオブジェクト＋自分オブジェクトの法線（長さ１）横の位置に設定
			trans[i - 1]->SetPosition({
				GetOwner()->GetComponent<Transform>()->GetPosition().x +
					GetOwner()->GetComponent<Transform>()->GetVectorRight().x +
					GetOwner()->GetComponent<Transform>()->GetVectorForword().x * ChargePer,
				GetOwner()->GetComponent<Transform>()->GetPosition().y +
					GetOwner()->GetComponent<Transform>()->GetVectorRight().y +
					GetOwner()->GetComponent<Transform>()->GetVectorForword().y * ChargePer,
				GetOwner()->GetComponent<Transform>()->GetPosition().z +
					GetOwner()->GetComponent<Transform>()->GetVectorRight().z +
					GetOwner()->GetComponent<Transform>()->GetVectorForword().z * ChargePer
				});
			// 角度を自分のオブジェクトの角度に設定
			trans[i - 1]->SetAngle({
				GetOwner()->GetComponent<Transform>()->GetAngle().x,
				GetOwner()->GetComponent<Transform>()->GetAngle().y + 60.0f,// 矢のモデルと矢を射出するモデルの正面が違う場合、ここで数値調整する。
				GetOwner()->GetComponent<Transform>()->GetAngle().z
				});
			// チャージタイム以上に長押ししていた場合
			if (m_tic > m_ChargeTime) {
				// サイズを設定
				trans[i - 1]->SetScale({ 0.6f, 0.6f, 0.6f });
				rb[i - 1]->SetDrag(1.0f);
				rb[i - 1]->SetMass(0.01f);

				m_haveArrow[i]->GetComponent<ArrowController>()->SetArrowType(ArrowController::ARROW_TYPE::SUPER);
			}
			// 通常の場合
			else {
				// サイズを設定
				trans[i - 1]->SetScale({ 0.3f, 0.3f, 0.3f });
				rb[i - 1]->SetDrag(1.0f);
				m_haveArrow[i]->GetComponent<ArrowController>()->SetArrowType(ArrowController::ARROW_TYPE::NORMAL);
			}

			// 溜め中なので加速度を0.0fに設定
			rb[i - 1]->SetAccele({ 0.0f, 0.0f, 0.0f });
		}
	}

	//スペースを放したとき
	if (IsKeyRelease(VK_SPACE)
		|| XInput::GetJoyRelease(0, BUTTON_TYPE::R)) {
		//ゲージのリセット
		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(0);
		// 変更用ポインタ
		std::shared_ptr<Rigidbody> rb[2];

		// 今持っているArrowのRigidbodyを取得
		rb[0] = m_haveArrow[1]->GetComponent<Rigidbody>();
		rb[1] = m_haveArrow[2]->GetComponent<Rigidbody>();
		// チャージタイム以上に長押ししていた場合
		if (m_tic > m_ChargeTime) {
			// 加速度をオブジェクトの正面方向に設定
			rb[0]->SetAccele({
				(GetOwner()->GetComponent<Transform>()->GetVectorForword().x +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().x * 0.857f) * 0.6f,
				(GetOwner()->GetComponent<Transform>()->GetVectorForword().y +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().y * 0.857f) * 0.6f,
				(GetOwner()->GetComponent<Transform>()->GetVectorForword().z +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().z * 0.857f) * 0.6f
				});
			rb[1]->SetAccele({
				(GetOwner()->GetComponent<Transform>()->GetVectorForword().x +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().x * -0.857f) * 0.6f,
				(GetOwner()->GetComponent<Transform>()->GetVectorForword().y +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().y * -0.857f) * 0.6f,
				(GetOwner()->GetComponent<Transform>()->GetVectorForword().z +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().z * -0.857f) * 0.6f
				});

			// 自分に撃った反動を加える
			m_bKnockBackFlg = true;
			GetOwner()->GetComponent<Rigidbody>()->SetAccele({
				GetOwner()->GetComponent<Transform>()->GetVectorForword().x * -m_KnockBackPower,
				GetOwner()->GetComponent<Transform>()->GetVectorForword().y * -m_KnockBackPower,
				GetOwner()->GetComponent<Transform>()->GetVectorForword().z * -m_KnockBackPower
				});
		}
		// 通常の場合
		else {
			// 加速度をオブジェクトの正面方向に設定
			rb[0]->SetAccele({
				(GetOwner()->GetComponent<Transform>()->GetVectorForword().x +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().x * 0.857f) * 0.3f,
				(GetOwner()->GetComponent<Transform>()->GetVectorForword().y +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().y * 0.857f) * 0.3f,
				(GetOwner()->GetComponent<Transform>()->GetVectorForword().z +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().z * 0.857f) * 0.3f
				});
			rb[1]->SetAccele({
				(GetOwner()->GetComponent<Transform>()->GetVectorForword().x +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().x * -0.857f) * 0.3f,
				(GetOwner()->GetComponent<Transform>()->GetVectorForword().y +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().y * -0.857f) * 0.3f,
				(GetOwner()->GetComponent<Transform>()->GetVectorForword().z +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().z * -0.857f) * 0.3f
				});
		}
		// 矢を離したためポインタをnullptrにする
		for (int i = 1; i < 3; i++)
		{
			m_haveArrow[i] = nullptr;
		}

		//スペシャル回数のカウントダウン
		m_Specialcnt--;
		if (m_Specialcnt == 4)		ObjectManager::FindObjectWithName("UI.13")->GetComponent<SpesialGauge>()->Swapframe(4);
		if (m_Specialcnt == 3)		ObjectManager::FindObjectWithName("UI.13")->GetComponent<SpesialGauge>()->Swapframe(3);
		if (m_Specialcnt == 2)		ObjectManager::FindObjectWithName("UI.13")->GetComponent<SpesialGauge>()->Swapframe(2);
		if (m_Specialcnt == 1)		ObjectManager::FindObjectWithName("UI.13")->GetComponent<SpesialGauge>()->Swapframe(1);
		if (m_Specialcnt == 0)
		{//スペシャルの終了とゲージのリセット
			ObjectManager::FindObjectWithName("UI.13")->GetComponent <SpesialGauge>()->Swapframe(0);
			SetEnableSpecial(false);
		}
	}


}



//===============
//ノーマルの矢
//===============
void PlayerController::IsNormalArrow()
{
	// 矢の発射　単発うち
	if (IsKeyTrigger(VK_SPACE)
		|| XInput::GetJoyTrigger(0, BUTTON_TYPE::R)) {
		m_tic = 0.0f;	// 押し始めたら、0.0fに初期化

		// 変更用ポインタ
		std::shared_ptr<Transform> trans;
		std::shared_ptr<Rigidbody> rb;

		//--- オブジェクト作成
		//   型　：Arrow
		//  名前 ：Arrow
		// タグ名：Arrow
		m_haveArrow[0] = ObjectManager::CreateObject<Arrow>("Arrow", TagName::Arrow);
		// 今持っているArrowのTransformを取得
		trans = m_haveArrow[0]->GetComponent<Transform>();
		// 座標を自分のオブジェクト＋自分オブジェクトの法線（長さ１）横の位置に設定
		// 要約：右前
		trans->SetPosition({
			GetOwner()->GetComponent<Transform>()->GetPosition().x +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().x +
				GetOwner()->GetComponent<Transform>()->GetVectorForword().x,
			GetOwner()->GetComponent<Transform>()->GetPosition().y +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().y +
				GetOwner()->GetComponent<Transform>()->GetVectorForword().y,
			GetOwner()->GetComponent<Transform>()->GetPosition().z +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().z +
				GetOwner()->GetComponent<Transform>()->GetVectorForword().z
			});
		// 角度を自分のオブジェクトの角度に設定
		trans->SetAngle({
			GetOwner()->GetComponent<Transform>()->GetAngle().x,
			GetOwner()->GetComponent<Transform>()->GetAngle().y + 0.0f,// 矢のモデルと矢を射出するモデルの正面が違う場合、ここで数値調整する。
			GetOwner()->GetComponent<Transform>()->GetAngle().z
			});
	}

	//スペースを押している間　チャージ中
	if (IsKeyPress(VK_SPACE)
		|| XInput::GetJoyButton(0, BUTTON_TYPE::R)) {
		//UIのuv座標の切り替え
		ObjectManager::FindObjectWithName("UI.8")->GetComponent<clicAtk>()->Swapframe(1);
		ObjectManager::FindObjectWithName("UI.8")->GetComponent<clicAtk>()->Play();

		m_tic++;	// 押している間、カウントする
		// 変更用ポインタ
		std::shared_ptr<Transform> trans;
		std::shared_ptr<Rigidbody> rb;
		// 今持っているArrowのTransformを取得
		trans = m_haveArrow[0]->GetComponent<Transform>();
		// 今持っているArrowのRigidbodyを取得
		rb = m_haveArrow[0]->GetComponent<Rigidbody>();
		// チャージ時間の割合を求める
		float ChargePer = m_tic > m_ChargeTime ? m_ChargeTime / m_ChargeTime : m_tic / m_ChargeTime;

		//
		int SwapGauge = ChargePer / (1.0f / 16.0f);
		if (SwapGauge == 0) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(0);
		if (SwapGauge == 1) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(1);
		if (SwapGauge == 2) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(2);
		if (SwapGauge == 3) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(3);
		if (SwapGauge == 4) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(4);
		if (SwapGauge == 5) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(5);
		if (SwapGauge == 6) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(6);
		if (SwapGauge == 7) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(7);
		if (SwapGauge == 8) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(8);
		if (SwapGauge == 9) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(9);
		if (SwapGauge == 10)		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(10);
		if (SwapGauge == 11) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(11);
		if (SwapGauge == 12) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(12);
		if (SwapGauge == 13) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(13);
		if (SwapGauge == 14) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(14);
		if (SwapGauge == 15) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(15);
		if (SwapGauge == 16) 		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(16);

		// 割合を逆にする
		ChargePer = 1 - ChargePer;
		// 座標を自分のオブジェクト＋自分オブジェクトの法線（長さ１）横の位置に設定
		trans->SetPosition({
			GetOwner()->GetComponent<Transform>()->GetPosition().x +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().x +
				GetOwner()->GetComponent<Transform>()->GetVectorForword().x * ChargePer,
			GetOwner()->GetComponent<Transform>()->GetPosition().y +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().y +
				GetOwner()->GetComponent<Transform>()->GetVectorForword().y * ChargePer,
			GetOwner()->GetComponent<Transform>()->GetPosition().z +
				GetOwner()->GetComponent<Transform>()->GetVectorRight().z +
				GetOwner()->GetComponent<Transform>()->GetVectorForword().z * ChargePer
			});
		// 角度を自分のオブジェクトの角度に設定
		trans->SetAngle({
			GetOwner()->GetComponent<Transform>()->GetAngle().x,
			GetOwner()->GetComponent<Transform>()->GetAngle().y - 90.0f,// 矢のモデルと矢を射出するモデルの正面が違う場合、ここで数値調整する。
			GetOwner()->GetComponent<Transform>()->GetAngle().z
			});
		// チャージタイム以上に長押ししていた場合
		if (m_tic > m_ChargeTime) {
			// サイズを設定
			trans->SetScale({ 0.6f, 0.6f, 0.6f });
			rb->SetDrag(1.0f);
			rb->SetMass(0.01f);

			m_haveArrow[0]->GetComponent<ArrowController>()->SetArrowType(ArrowController::ARROW_TYPE::SUPER);
		}
		// 通常の場合
		else {
			// サイズを設定
			trans->SetScale({ 0.3f, 0.3f, 0.3f });
			rb->SetDrag(1.0f);
			m_haveArrow[0]->GetComponent<ArrowController>()->SetArrowType(ArrowController::ARROW_TYPE::NORMAL);
		}

		// 溜め中なので加速度を0.0fに設定
		rb->SetAccele({ 0.0f, 0.0f, 0.0f });
	}

	//スペースを放したとき　チャージした矢の発射
	if (IsKeyRelease(VK_SPACE)
		|| XInput::GetJoyRelease(0, BUTTON_TYPE::R)) {
		//ゲージのリセット
		ObjectManager::FindObjectWithName("UI.7")->GetComponent<AtkGauge>()->Swapframe(0);
		// 変更用ポインタ
		std::shared_ptr<Rigidbody> rb;
		// 今持っているArrowのRigidbodyを取得
		rb = m_haveArrow[0]->GetComponent<Rigidbody>();
		// チャージタイム以上に長押ししていた場合
		if (m_tic > m_ChargeTime) {
			// 加速度をオブジェクトの正面方向に設定
			rb->SetAccele({
				GetOwner()->GetComponent<Transform>()->GetVectorForword().x * 0.6f,
				GetOwner()->GetComponent<Transform>()->GetVectorForword().y * 0.6f,
				GetOwner()->GetComponent<Transform>()->GetVectorForword().z * 0.6f
				});
			// 自分に撃った反動を加える
			m_bKnockBackFlg = true;
			GetOwner()->GetComponent<Rigidbody>()->SetAccele({
				GetOwner()->GetComponent<Transform>()->GetVectorForword().x * -m_KnockBackPower,
				GetOwner()->GetComponent<Transform>()->GetVectorForword().y * -m_KnockBackPower,
				GetOwner()->GetComponent<Transform>()->GetVectorForword().z * -m_KnockBackPower
				});
		}
		// 通常の場合
		else {
			// 加速度をオブジェクトの正面方向に設定
			rb->SetAccele({
				GetOwner()->GetComponent<Transform>()->GetVectorForword().x * 0.3f,
				GetOwner()->GetComponent<Transform>()->GetVectorForword().y * 0.3f,
				GetOwner()->GetComponent<Transform>()->GetVectorForword().z * 0.3f
				});
		}
		// 矢を離したためポインタをnullptrにする
		m_haveArrow[0] = nullptr;

		//仮配置<UI切り替え>ーーーーーーーーーーーーー
				//UIのuv座標の切り替え
		ObjectManager::FindObjectWithName("UI.8")->GetComponent<clicAtk>()->Swapframe(0);
		ObjectManager::FindObjectWithName("UI.8")->GetComponent<clicAtk>()->Play();
	}
}

void PlayerController::SetEnableSpecial(bool enable)
{
	m_EnableSpecial = enable;
	if (enable)
	{
		m_Specialcnt = 4;
		ObjectManager::FindObjectWithName("UI.13")->GetComponent<SpesialGauge>()->Swapframe(4);
	}
}