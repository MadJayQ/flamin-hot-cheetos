#include "SDK.h"

Tools tools;

void* Tools::getInterface(std::string moduleName, std::string interfaceName)
{
	typedef void* (*CreateInterfaceFn)(const char* name, int* returnCode);
	CreateInterfaceFn CreateInterface = nullptr;

	while (!CreateInterface)
	{
		CreateInterface = (CreateInterfaceFn)GetProcAddress(GetModuleHandleA(moduleName.c_str()), charenc("CreateInterface"));
		Sleep(5);
	}

	char buffer[256];

	for (int i = 0; i < 100; i++)
	{
		sprintf_s(buffer, "%s%0.3d", interfaceName.c_str(), i);
		void* interface = CreateInterface(buffer, nullptr);

		if (interface && interface != nullptr)
			break;
	}

	return CreateInterface(buffer, nullptr);
}

bool Tools::isVisible(Vector& start, Vector& end, CBaseEntity* entity)
{
	IEngineTrace::trace_t trace;
	IEngineTrace::Ray_t ray;
	IEngineTrace::CTraceFilter filter;
	filter.skip = interfaces::entitylist->GetClientEntity(interfaces::engine->GetLocalPlayer());

	ray.Init(start, end);
	interfaces::enginetrace->TraceRay(ray, 0x4600400B, &filter, &trace);

	return (trace.entity == entity || trace.fraction > 0.99f);
}

CBaseCombatWeapon* Tools::getActiveWeapon(CBaseEntity* entity)
{
	ULONG weaponHandle = (ULONG)*(DWORD*)((DWORD)entity + 0x2EE8);
	return (CBaseCombatWeapon*)interfaces::entitylist->GetClientEntityFromHandle(weaponHandle);
}

bool Tools::WorldToScreen(Vector& world, Vector& screen)
{
	return (interfaces::debugoverlay->ScreenPosition(world, screen) != 1);
}

float DotProductFloat(const float* v1, const float* v2)
{
	return (v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2]);
}

void VectorTransformFloat(const float* in1, const matrix3x4& in2, float* out)
{
	Assert(in1 != out);
	out[0] = DotProductFloat(in1, in2[0]) + in2[0][3];
	out[1] = DotProductFloat(in1, in2[1]) + in2[1][3];
	out[2] = DotProductFloat(in1, in2[2]) + in2[2][3];
}

void Tools::VectorTransform(const Vector& in1, const matrix3x4& in2, Vector& out)
{
	VectorTransformFloat(&in1.x, in2, &out.x);
}

void Tools::sinCos(float radians, float* sine, float* cosine)
{
	_asm
	{
		fld		DWORD PTR[radians]
		fsincos

			mov edx, DWORD PTR[cosine]
			mov eax, DWORD PTR[sine]

			fstp DWORD PTR[edx]
			fstp DWORD PTR[eax]
	}
}

void Tools::angleVectors(const Vector angles, Vector& forward)
{
	float sp, sy, cp, cy;
	sinCos(DEG2RAD(angles[1]), &sy, &cy);
	sinCos(DEG2RAD(angles[0]), &sp, &cp);

	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;
}

bool Tools::getHitboxPosition(int hitbox, Vector& output, CBaseEntity* entity)
{
	if (hitbox >= 22)
		return false;

	const model_t* model = entity->GetModel();
	if (model)
	{
		studiohdr_t* studioHdr = interfaces::modelinfo->GetStudioModel(model);
		if (!studioHdr)
			return false;

		matrix3x4 matrix[128];
		if (!entity->SetupBones(matrix, 128, 0x100, interfaces::engine->GetLastTimeStamp()))
			return false;

		mstudiobbox_t* box = studioHdr->pHitbox(hitbox, 0);
		if (!box)
			return false;

		Vector min, max;
		VectorTransform(box->bbmin, matrix[box->bone], min);
		VectorTransform(box->bbmax, matrix[box->bone], max);
		output = (min + max) * 0.5f;

		return true;
	}

	return false;
}

float Tools::getDistance(Vector origin, Vector other)
{
	float distance = sqrt((origin - other).Length());
	if (distance < 1.f)
		distance = 1.f;

	return distance;
}

float Tools::getFov(QAngle viewAngles, QAngle aimAngles)
{
	Vector ang, aim;
	angleVectors(viewAngles, aim);
	angleVectors(aimAngles, ang);

	return RAD2DEG(acos(aim.Dot(ang) / aim.LengthSqr()));
}

void Tools::computeAngle(Vector source, Vector dest, QAngle& angles)
{
	QAngle delta = source - dest;
	angles.x = (float)(asinf(delta.z / delta.Length()) * M_RADPI);
	angles.y = (float)(atanf(delta.y / delta.x) * M_RADPI);
	angles.z = 0.f;

	if (delta.x >= 0.f)
		angles.y += 180.f;
}

QAngle Tools::computeAngle(Vector source, Vector dest)
{
	QAngle angles;

	Vector delta = source - dest;
	angles.x = (float)(asinf(delta.z / delta.Length()) * M_RADPI);
	angles.y = (float)(atanf(delta.y / delta.x) * M_RADPI);
	angles.z = 0.f;

	if (delta.x >= 0.f)
		angles.y += 180.f;

	return angles;
}

void Tools::normalizeAngles(QAngle& angles)
{
	for (int i = 0; i < 2; ++i)
	{
		while (angles[i] > 180.f)
			angles[i] -= 360.f;

		while (angles[i] < -180.f)
			angles[i] += 360.f;
	}

	angles[2] = 0.f;
}

void Tools::clampAngles(QAngle& angles)
{
	if (angles.x > 89.f)
		angles.x = 89.f;
	else if (angles.x < -89.f)
		angles.x = -89.f;

	if (angles.y > 180.f)
		angles.y = 180.f;
	else if (angles.y < -180.f)
		angles.y = -180.f;

	angles.z = 0.f;
}

int Tools::random(int min, int max)
{
	return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

float Tools::random(float min, float max)
{
	return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}