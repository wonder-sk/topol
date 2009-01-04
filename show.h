#include <qgsgeometry.h>
#include <qgspoint.h>

void show_line(QgsPolyline line) {
	std::cout << "\nLine:\n";

	QgsPolyline::ConstIterator it = line.begin();	
	for (; it != line.end(); ++it)
		std::cout << it->toString().toStdString() << " | ";

	std::cout << "\nEOLine\n";
}
