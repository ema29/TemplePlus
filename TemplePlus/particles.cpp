
#include "stdafx.h"
#include "particles.h"

Particles particles;

static struct ParticleAddresses : temple::AddressTable {

	uint32_t (__cdecl *Elf32Hash)(const char *name);
	int (__cdecl *CreateAtObj)(uint32_t nameHash, objHndl handle);
	int (__cdecl *CreateAtPos)(uint32_t nameHash, vector3f pos);
	void (__cdecl *End)(int partSysId);
	void (__cdecl *Kill)(int partSysId);
	void (__cdecl *CallLightning)(LocAndOffsets location);
	void (__cdecl *ChainLightning)(objHndl caster, int targetCount, const objHndl *targets);
	void (__cdecl *LightningBolt)(objHndl caster, LocAndOffsets target);

	ParticleAddresses() {
		rebase(CreateAtObj, 0x10049B70);
		rebase(CreateAtPos, 0x10049BD0);
		rebase(Elf32Hash, 0x101EBB00);
		rebase(End, 0x10049BF0);
		rebase(Kill, 0x10049BE0);
		rebase(CallLightning, 0x10087440);
		rebase(ChainLightning, 0x10087480);
		rebase(LightningBolt, 0x100875C0);
	}

} addresses;

int Particles::CreateAtObj(const char *name, objHndl atObj) {
	auto nameHash = addresses.Elf32Hash(name);
	return addresses.CreateAtObj(nameHash, atObj);
}

int Particles::CreateAt3dPos(const char *name, vector3f pos) {
	auto nameHash = addresses.Elf32Hash(name);
	return addresses.CreateAtPos(nameHash, pos);
}

void Particles::Kill(int partSysId) {
	addresses.Kill(partSysId);
}

void Particles::End(int partSysId) {
	addresses.End(partSysId);
}

void Particles::CallLightning(LocAndOffsets location) {
	addresses.CallLightning(location);
}

void Particles::LightningBolt(objHndl caster, LocAndOffsets target) {
	addresses.LightningBolt(caster, target);
}

void Particles::ChainLightning(objHndl caster, const vector<objHndl>& targets) {
	addresses.ChainLightning(caster, targets.size(), targets.data());
}
