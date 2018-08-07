
#ifndef __RENDERDEBUG_H__
#define __RENDERDEBUG_H__

class idRenderDebug {
public:
	virtual void DebugClear( uint32 deltaTime ) = 0;
	virtual void DebugLine() = 0;
	virtual void DebugArrow() = 0;
	virtual void DebugArrow2() = 0;
	virtual void DebugArrow3() = 0;
	virtual void DebugPyramid() = 0;
	virtual void DebugWinding() = 0;
	virtual void DebugCircle() = 0;
	virtual void DebugCylinder() = 0;
	virtual void DebugArc() = 0;
	virtual void DebugShadedArc() = 0;
	virtual void DebugSphere() = 0;
	virtual void DebugBounds() = 0;
	virtual void DebugOrientedBounds() = 0;
	virtual void DebugBox() = 0;
	virtual void DebugCone() = 0;
	virtual void DebugAxis() = 0;
	virtual void DebugAxisScaled() = 0;
	virtual void DebugPoint() = 0;
	virtual void DebugFilledPolygon() = 0;
	virtual void DebugFilledBounds() = 0;
	virtual void DebugText() = 0;
	virtual void DebugSpline() = 0;
};

#endif /*__RENDERDEBUG_H__*/
