#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "../platform/d3d.h"

enum AasEventFlag;

namespace gfx {

	struct AnimatedModelParams;
	using AnimatedModelPtr = std::shared_ptr<class AnimatedModel>;

	enum class WeaponAnim : int {
		None = 0,
		RightAttack,
		RightAttack2,
		RightAttack3,
		LeftAttack,
		LeftAttack2,
		LeftAttack3,
		Walk,
		Run,
		Idle,
		FrontHit,
		FrontHit2,
		FrontHit3,
		LeftHit,
		LeftHit2,
		LeftHit3,
		RightHit,
		RightHit2,
		RightHit3,
		BackHit,
		BackHit2,
		BackHit3,
		RightCriticalSwing,
		LeftCriticalSwing,
		Fidget,
		Fidget2,
		Fidget3,
		Sneak,
		Panic,
		RightCombatStart,
		LeftCombatStart,
		CombatIdle,
		CombatFidget,
		Special1,
		Special2,
		Special3,
		FrontDodge,
		RightDodge,
		LeftDodge,
		BackDodge,
		RightThrow,
		LeftThrow,
		LeftSnatch,
		RightSnatch,
		LeftTurn,
		RightTurn
	};

	enum class WeaponAnimType : int {
		Unarmed,
		Dagger,
		Sword,
		Mace,
		Hammer,
		Axe,
		Club,
		Battleaxe,
		Greatsword,
		Greataxe,
		Greathammer,
		Spear,
		Staff,
		Polearm,
		Bow,
		Crossbow,
		Sling,
		Shield,
		Flail,
		Chain,
		TwoHandedFlail,
		Shuriken,
		Monk
	};

	/*
	Represents an encoded animation id.
	*/
	class EncodedAnimId {
	public:
		explicit EncodedAnimId(int id) : mId(id) {
		}

		explicit EncodedAnimId(WeaponAnim anim,
		                       WeaponAnimType leftHand = WeaponAnimType::Unarmed,
		                       WeaponAnimType rightHand = WeaponAnimType::Unarmed) : mId(sWeaponAnimFlag) {
			auto animId = (int)anim;
			auto leftHandId = (int)leftHand;
			auto rightHandId = (int)rightHand;

			mId |= animId & 0xFFFFF;
			mId |= leftHandId << 20;
			mId |= rightHandId << 25;
		}

		operator int() const {
			return mId;
		}

	private:
		// Indicates that an animation id uses the encoded format
		static constexpr int sWeaponAnimFlag = 1 << 30;

		int mId;
	};

	/*
		Represents the events that can trigger when the animation
		of an animated model is advanced.
	*/
	class AnimatedModelEvents {
	public:
		AnimatedModelEvents(bool isEnd, bool isAction)
			: mIsEnd(isEnd),
			  mIsAction(isAction) {
		}

		bool IsEnd() const {
			return mIsEnd;
		}

		bool IsAction() const {
			return mIsAction;
		}

	private:
		const bool mIsEnd : 1;
		const bool mIsAction : 1;
	};

	class Submesh {
	public:
		virtual ~Submesh() {
		}

		virtual int GetVertexCount() = 0;
		virtual int GetPrimitiveCount() = 0;
		virtual float* GetPositions() = 0;
		virtual float* GetNormals() = 0;
		virtual float* GetUV() = 0;
		virtual uint16_t* GetIndices() = 0;
	};

	class AnimatedModel {
	public:
		virtual ~AnimatedModel() {
		}

		virtual bool AddAddMesh(const std::string& filename) = 0;

		virtual bool ClearAddMeshes() = 0;

		virtual AnimatedModelEvents Advance(float deltaTime,
		                                    float deltaDistance,
		                                    float deltaRotation,
		                                    const AnimatedModelParams& params) = 0;

		virtual EncodedAnimId GetAnimId() const = 0;

		virtual int GetBoneCount() const = 0;

		virtual std::string GetBoneName(int boneId) = 0;

		virtual int GetBoneParentId(int boneId) = 0;

		virtual bool GetBoneWorldMatrixByName(
			const AnimatedModelParams& params,
			const std::string& boneName,
			D3DMATRIX* worldMatrixOut) = 0;

		virtual bool GetBoneWorldMatrixByNameForChild(const AnimatedModelPtr& child,
		                                              const AnimatedModelParams& params,
		                                              const std::string& boneName,
		                                              D3DMATRIX* worldMatrixOut) = 0;


		virtual float GetDistPerSec() const = 0;

		virtual float GetRotationPerSec() const = 0;

		virtual bool HasAnim(EncodedAnimId animId) const = 0;

		virtual void SetTime(const AnimatedModelParams& params, float timeInSecs) = 0;

		virtual bool HasBone(const std::string& boneName) const = 0;

		virtual void AddReplacementMaterial(int materialId) = 0;

		virtual void SetAnimId(int animId) = 0;

		virtual void SetClothFlag() = 0;

		virtual std::vector<int> GetSubmeshes() = 0;

		virtual std::unique_ptr<Submesh> GetSubmesh(const AnimatedModelParams& params, int submeshIdx) = 0;

		virtual std::unique_ptr<Submesh> GetSubmeshForParticles(const AnimatedModelParams& params, int submeshIdx) = 0;

	};

	struct AnimatedModelParams {
		uint32_t x = 0;
		uint32_t y = 0;
		float offsetX = 0;
		float offsetY = 0;
		float offsetZ = 0;
		float rotation = 0;
		float scale = 1;
		float rotationRoll = 0;
		float rotationPitch = 0;
		float rotationYaw = 0;
		AnimatedModelPtr parentAnim;
		std::string attachedBoneName;
	};

	class AnimatedModelFactory {
	public:
		virtual ~AnimatedModelFactory() {
		}

		virtual AnimatedModelPtr FromIds(
			int meshId,
			int skeletonId,
			EncodedAnimId idleAnimId,
			const AnimatedModelParams& params) = 0;

		virtual AnimatedModelPtr FromFilenames(
			const std::string& meshFilename,
			const std::string& skeletonFilename,
			EncodedAnimId idleAnimId,
			const AnimatedModelParams& params) = 0;
	};

	class Mesh {
	public:
		Mesh(const std::string& name, bool valid) : mName(name),
		                                            mValid(valid) {
		}

		bool IsValid() const {
			return mValid;
		}

		std::string GetName() const {
			return mName;
		}

		int GetLegacyId() const {
			return mLegacyId;
		}

		void SetLegacyId(int legacyId) {
			mLegacyId = legacyId;
		}

	private:
		const std::string mName;
		const bool mValid;
		int mLegacyId = -1;
	};

	using MeshRef = std::shared_ptr<Mesh>;

	class MeshesManager {
	public:

		MeshRef Resolve(int meshId);

		MeshRef Resolve(const std::string& name);

		void LoadMapping(const std::string& filename);

	private:

		std::unordered_map<std::string, MeshRef> mMeshesByName;

		// This is the mapping loaded from meshes.mes
		std::unordered_map<int, std::string> mMapping;

	};

	using MeshesManagerPtr = std::shared_ptr<MeshesManager>;

}
