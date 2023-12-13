
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"

extern BasicCamera		light;
extern BasicCamera		debugcamera;

extern OpenGLEffect*	simplecolor;
extern OpenGLMesh*		unitcube;
extern OpenGLMesh*		debugbox;
extern OpenGLMesh*		debugline;

void DebugRender(
	const Math::Matrix& viewproj, const Math::Matrix& lightviewproj, const Math::Vector3& eyepos,
	const Math::AABox& scenebox, const Math::AABox& body, const std::vector<Math::Vector3>& points)
{
	Math::Matrix lightview, lightviewinv;
	Math::Matrix debugview, debugproj;
	Math::Matrix debugworld;
	Math::Matrix debugviewproj;
	Math::Vector4 debugcolor(0, 1, 0, 1);
	Math::Vector3 halfsize;
	Math::Vector3 center;

	light.GetViewMatrix(lightview);

	lightview._41 = 0;
	lightview._42 = 0;
	lightview._43 = 0;

	Math::MatrixInverse(lightviewinv, lightview);

	debugcamera.GetViewMatrix(debugview);
	debugcamera.GetProjectionMatrix(debugproj);

	Math::MatrixInverse(debugworld, viewproj);
	Math::MatrixMultiply(debugviewproj, debugview, debugproj);

	simplecolor->SetMatrix("matWorld", debugworld);
	simplecolor->SetMatrix("matViewProj", debugviewproj);
	simplecolor->SetVector("matColor", &debugcolor.x);

	simplecolor->Begin();
	{
		// draw view frustum (green)
		debugbox->DrawSubset(0);

		// draw scene envelope (red)
		scenebox.GetHalfSize(halfsize);
		scenebox.GetCenter(center);

		Math::MatrixScaling(debugworld, halfsize.x, halfsize.y, halfsize.z);

		debugworld._41 = center.x;
		debugworld._42 = center.y;
		debugworld._43 = center.z;

		debugcolor = Math::Vector4(1, 0, 0, 1);

		simplecolor->SetMatrix("matWorld", debugworld);
		simplecolor->SetVector("matColor", &debugcolor.x);
		simplecolor->CommitChanges();

		debugbox->DrawSubset(0);

		// draw light frustum (orange)
		Math::MatrixInverse(debugworld, lightviewproj);
		debugcolor = Math::Vector4(1, 0.5f, 0, 1);

		simplecolor->SetMatrix("matWorld", debugworld);
		simplecolor->SetVector("matColor", &debugcolor.x);
		simplecolor->CommitChanges();

		debugbox->DrawSubset(0);

		// draw intersection body bounding box (purple)
		body.GetHalfSize(halfsize);
		body.GetCenter(center);

		Math::MatrixScaling(debugworld, halfsize.x, halfsize.y, halfsize.z);

		debugworld._41 = center.x;
		debugworld._42 = center.y;
		debugworld._43 = center.z;

		debugcolor = Math::Vector4(1, 0, 1, 1);

		simplecolor->SetMatrix("matWorld", debugworld);
		simplecolor->SetVector("matColor", &debugcolor.x);
		simplecolor->CommitChanges();

		debugbox->DrawSubset(0);

		// draw intersection points (yellow)
		debugcolor = Math::Vector4(1, 1, 0, 1);

		Math::MatrixScaling(debugworld, 0.3f, 0.3f, 0.3f);

		for (const Math::Vector3& isect : points) {
			debugworld._41 = isect.x;
			debugworld._42 = isect.y;
			debugworld._43 = isect.z;

			simplecolor->SetMatrix("matWorld", debugworld);
			simplecolor->SetVector("matColor", &debugcolor.x);
			simplecolor->CommitChanges();

			unitcube->DrawSubset(0);
		}

		// draw lightdir from intersection points (yellow)
		Math::MatrixScaling(debugworld, 0, 0, 5);
		Math::MatrixMultiply(debugworld, debugworld, lightviewinv);

		for (const Math::Vector3& isect : points) {
			debugworld._41 = isect.x;
			debugworld._42 = isect.y;
			debugworld._43 = isect.z;

			simplecolor->SetMatrix("matWorld", debugworld);
			simplecolor->SetVector("matColor", &debugcolor.x);
			simplecolor->CommitChanges();

			debugline->DrawSubset(0);
		}

		// draw corrected light eye position (red)
		debugcolor = Math::Vector4(1, 0, 0, 1);

		Math::MatrixScaling(debugworld, 0.3f, 0.3f, 0.3f);

		debugworld._41 = eyepos.x;
		debugworld._42 = eyepos.y;
		debugworld._43 = eyepos.z;

		simplecolor->SetMatrix("matWorld", debugworld);
		simplecolor->SetVector("matColor", &debugcolor.x);
		simplecolor->CommitChanges();

		unitcube->DrawSubset(0);
	}
	simplecolor->End();
}
