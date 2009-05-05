#ifndef TOPOLERROR_H
#define TOPOLERROR_H

enum TopolErrorType
{
  TopolErrorIntersection = 1,
  TopolErrorSelfIntersection,
  TopolErrorOverlap,
  TopolErrorTolerance,
  TopolErrorDangle
};

enum TopolFixType
{
  TopolFixMoveFirst = QString(,
  TopolFixMoveSecond,
  TopolFixUnionFirst,
  TopolFixUnionSecond,
  TopolFixDeleteFirst,
  TopolFixDeleteSecond,
  TopolFixSnap,
};

//1;2A
//const StringList FixType = 

class TopolError
{
public:
  TopolErrorType type;
  QgsRectangle boundingBox;  
  QgsGeometry conflict;
  QgsFeatureIds fids;
  QList<TopolFixType> fixes;

  bool fix(TopolFixType type);
};

#endif
