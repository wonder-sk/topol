#include "topolError.h"

bool TopolError::fixIt(QString fixName)
{
  //fixFunction f = mFixMap[fixName];
  //(this->*f)();
  (this->*mFixMap[fixName])();
}

bool TopolError::fixMoveFirst()
{
}

bool TopolError::fixMoveSecond()
{
}

bool TopolError::fixUnionFirst()
{
}

bool TopolError::fixUnionSecond()
{
}

bool TopolError::fixDeleteFirst()
{
}

bool TopolError::fixDeleteSecond()
{
}

//TopolErrorIntersection
//TopolErrorIntersection::TopolErrorIntersection(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QgsFeatureIds theFids) : mBoundingBox(theBoundingBox), mConflict(theConflict), mFids(theFids)
TopolErrorIntersection::TopolErrorIntersection(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QgsFeatureIds theFids) : TopolError(theBoundingBox, theConflict, theFids)
{
  mBoundingBox = theBoundingBox;
  mConflict = theConflict;
  mFids = theFids;

  mFixMap["Move first feature"] = &TopolErrorIntersection::fixMoveFirst;
  mFixMap["Move second feature"] = &TopolErrorIntersection::fixMoveSecond;
  mFixMap["Union to first feature"] = &TopolErrorIntersection::fixUnionFirst;
  mFixMap["Union to second feature"] = &TopolErrorIntersection::fixUnionSecond;
  mFixMap["Delete first feature"] = &TopolErrorIntersection::fixDeleteFirst;
  mFixMap["Delete second feature"] = &TopolErrorIntersection::fixDeleteSecond;
}
