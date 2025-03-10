#ifndef ___ARROW_H___
#define ___ARROW_H___
#include "Object3D.h"
#include "MeshRenderer.h"
#include "Rigidbody.h"
#include "AABBCollider.h"
#include "ArrowController.h"

class Arrow : public Object3D
{
public:
	// コンストラクタ
	Arrow(std::string name, std::string tag) : Object3D(name, tag) {
		// 矢のモデルをロードする
		GetComponent<MeshRenderer>()->LoadModel("Assets/Model/arrow.fbx", 0.5f);
		// コンポーネントを追加
		AddComponent<Rigidbody>();
		AddComponent<ArrowController>();
		AddComponent<AABBCollider>();
		// 当たり判定の大きさを調整
		GetComponent<AABBCollider>()->SetLen({ 0.6f, 0.6f, 3.0f });
	}
	// デストラクタ
	~Arrow() {}
};

#endif //!___ARROW_H___