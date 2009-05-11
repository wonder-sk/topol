#include "topolError.h"

bool TopolError::fix(QString fixName)
{
std::cout << "fix: \""<<fixName.toStdString()<<"\"\n";
  (this->*mFixMap[fixName])();
}

bool TopolError::fixMoveFirst()
{
  return false;
}

bool TopolError::fixMoveSecond()
{
  return false;
}

bool TopolErrorDangle::fixSnap(int id1, int id2, QgsGeometry* g)
{
  bool ok;
  QgsFeature f1, f2;
  ok = mLayer->featureAtId(id1, f1, true, false);
  ok = ok && mLayer->featureAtId(id2, f2, true, false);

  if (!ok)
    return false;

  //snapToGeometry(point, geom, squaredTolerance, QMultiMap<double, QgsSnappingResult>, SnapToVertexAndSegment);

  //this shouldn't happen
  if (!g)
    return false;

  QgsGeometry* ge = f1.geometry();
  QgsGeometryV2 *gv2 = QgsGeometryV2::importFromOldGeometry(ge);

  LineString ls = gv2->lineString();
  ls.last() = g->asPolyline().last();
  ge = gv2->exportToOldGeometry();

  f1.setGeometry(ge);
  return true;
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
  {
    f1.setGeometry(g);
    return true;
  }

  return false;
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
//TODO: skip errors associated with deleted feature
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
  mFixMap["Move first feature"] = &TopolErrorIntersection::fixMoveFirst;
  mFixMap["Move second feature"] = &TopolErrorIntersection::fixMoveSecond;
  mFixMap["Union to first feature"] = &TopolErrorIntersection::fixUnionFirst;
  mFixMap["Union to second feature"] = &TopolErrorIntersection::fixUnionSecond;
  mFixMap["Delete first feature"] = &TopolErrorIntersection::fixDeleteFirst;
  mFixMap["Delete second feature"] = &TopolErrorIntersection::fixDeleteSecond;
}

TopolErrorDangle::TopolErrorDangle(QgsVectorLayer* theLayer, QgsRectangle theBoundingBox, QgsGeometry* theConflict, QgsFeatureIds theFids) : TopolError(theLayer, theBoundingBox, theConflict, theFids)
{
  mName = "Dangling endpoint";

  mFixMap["Select automatic fix"] = &TopolErrorDangle::fixDummy;
  mFixMap["Move first feature"] = &TopolErrorDangle::fixMoveFirst;
  mFixMap["Move second feature"] = &TopolErrorDangle::fixMoveSecond;
  mFixMap["Union to first feature"] = &TopolErrorDangle::fixSnap;
}
