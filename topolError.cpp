#include "topolError.h"

//TODO: fix crashing when no layer under
bool TopolError::fix(QString fixName)
{
std::cout << "fix: \""<<fixName.toStdString()<<"\"\n";
  (this->*mFixMap[fixName])();
}

bool TopolError::fixMove(int id1, int id2)
{
  bool ok;
  QgsFeature f1, f2;
  ok = mLayer->featureAtId(id1, f1, true, false);
  ok = ok && mLayer->featureAtId(id2, f2, true, false);

  if (!ok)
    return false;

  // 0 means success
  if(!f1.geometry()->makeDifference(f2.geometry()))
    return mLayer->changeGeometry(f1.id(), f1.geometry());

  return false;
}

bool TopolError::fixMoveFirst()
{
  return fixMove(mFids.values().first(), mFids.values()[1]);
}

bool TopolError::fixMoveSecond()
{
  return fixMove(mFids.values()[1], mFids.values().first());
}

bool TopolError::fixUnion(int id1, int id2)
{
  bool ok;
  QgsFeature f1, f2;
  ok = mLayer->featureAtId(id1, f1, true, false);
  ok = ok && mLayer->featureAtId(id2, f2, true, false);

  if (!ok)
    return false;

  QgsGeometry* g = f1.geometry()->combine(f2.geometry());
  if (!g)
    return false;

  if (mLayer->deleteFeature(f2.id()))
    return mLayer->changeGeometry(f1.id(), g);

  return false;
}

bool TopolError::fixSnap()
{
  bool ok;
  QgsFeature f1, f2;
  ok = mLayer->featureAtId(mFids.values().first(), f1, true, false);
  ok = ok && mLayer->featureAtId(mFids.values()[1], f2, true, false);

  if (!ok)
    return false;

  //snapToGeometry(point, geom, squaredTolerance, QMultiMap<double, QgsSnappingResult>, SnapToVertexAndSegment);

  QgsGeometry* ge = f1.geometry();

  QgsPolyline line = ge->asPolyline();
  line.last() = mConflict->asPolyline().last();

  //TODO: this will cause memory leaks
  return mLayer->changeGeometry(f1.id(), QgsGeometry::fromPolyline(line));
}

bool TopolError::fixUnionFirst()
{
  return fixUnion(mFids.values().first(), mFids.values()[1]);
}

bool TopolError::fixUnionSecond()
{
  return fixUnion(mFids.values()[1], mFids.values().first());
}

bool TopolError::fixDeleteFirst()
{
//TODO: skips errors associated with deleted feature
  return mLayer->deleteFeature(mFids.values().first());
}

bool TopolError::fixDeleteSecond()
{
  return mLayer->deleteFeature(mFids.values()[1]);
}

TopolErrorIntersection::TopolErrorIntersection(QgsVectorLayer* theLayer, QgsRectangle theBoundingBox, QgsGeometry* theConflict, QgsFeatureIds theFids) : TopolError(theLayer, theBoundingBox, theConflict, theFids)
{
  mName = "Intersecting geometries";

  mFixMap["Select automatic fix"] = &TopolErrorIntersection::fixDummy;
  mFixMap["Move blue feature"] = &TopolErrorIntersection::fixMoveFirst;
  mFixMap["Move red feature"] = &TopolErrorIntersection::fixMoveSecond;
  mFixMap["Union to blue feature"] = &TopolErrorIntersection::fixUnionFirst;
  mFixMap["Union to red feature"] = &TopolErrorIntersection::fixUnionSecond;
  mFixMap["Delete blue feature"] = &TopolErrorIntersection::fixDeleteFirst;
  mFixMap["Delete red feature"] = &TopolErrorIntersection::fixDeleteSecond;
}

TopolErrorDangle::TopolErrorDangle(QgsVectorLayer* theLayer, QgsRectangle theBoundingBox, QgsGeometry* theConflict, QgsFeatureIds theFids) : TopolError(theLayer, theBoundingBox, theConflict, theFids)
{
  mName = "Dangling endpoint";

  mFixMap["Select automatic fix"] = &TopolErrorDangle::fixDummy;
  mFixMap["Move blue feature"] = &TopolErrorDangle::fixMoveFirst;
  mFixMap["Move red feature"] = &TopolErrorDangle::fixMoveSecond;
  mFixMap["Snap to segment"] = &TopolErrorDangle::fixSnap;
}
