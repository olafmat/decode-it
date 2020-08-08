#include <algorithm>
#include <stack>
#include <vector>
#include <iostream>
using namespace std;

typedef long double real;

const real epsilon = (real)1e-20;

//Graham algorithm based on https://www.tutorialspoint.com/cplusplus-program-to-implement-graham-scan-algorithm-to-find-the-convex-hull

struct Circle {
    real x, y, r;
};

struct Point {
    real x, y;
    Circle *circle;
    real angle;
};

Point* point0;

Point* extractSecond(stack<Point*> &s) {
    Point* aux = s.top();
    s.pop();
    Point* res = s.top();
    s.push(aux);
    return res;
}

real dist2(const Point* a, const Point* b) {
    real dx = a -> x - b -> x;
    real dy = a -> y - b -> y;
    return dx * dx + dy * dy;
}

real direction(const Point* a, const Point* b, const Point* c) {
   return (b -> y - a -> y) * (c -> x - b -> x) - (b -> x - a -> x) * (c -> y - b -> y);
}

bool comparator(Point* &point1, Point* &point2) {
   real dir = direction(point0, point1, point2);
   //if (dir == (real)0) {
     //  return dist2(point0, point2) > dist2(point0, point1);
   //}
   return dir < -epsilon;
}

void findConvexHull(vector<Point*> &res, vector<Point*> &points) {
   real minY = points[0] -> y;
   int minI = 0;
   int n = points.size();
   for (int i = 1; i < n; i++) {
      real y = points[i] -> y;
      if (y < minY || (minY == y && points[i] -> x < points[minI] -> x)) {
         minY = points[i] -> y;
         minI = i;
      }
   }

   swap(points[0], points[minI]);
   point0 = points[0];
   sort(points.begin() + 1, points.end(), comparator);    //sort points from 1 place to end
   stack<Point*> s;

   for (int i = 0; i < 3 && i < n; i++) {
       s.push(points[i]);
   }

   for (int i = 3; i < n; i++) {
       while (direction(extractSecond(s), s.top(), points[i]) > (real)epsilon) {
           s.pop();
       }
       s.push(points[i]);
   }

   while (!s.empty()) {
       res.push_back(s.top());
       s.pop();
   }
}

void outerTangle() {
}

int main() {
   Point arr[] = {
      {-7,8},{-4,6},{2,6},{6,4},{8,6},{7,-2},{4,-6},{8,-7},{0,0},
      {3,-2},{6,-10},{0,-6},{-9,-5},{-8,-2},{-8,0},{-10,3},{-2,2},{-10,4},
      //{-7,8},{-4,6},{2,6},{6,4},{8,6},{7,-2},{4,-6},{8,-7},{0,0},
      //{3,-2},{6,-10},{0,-6},{-9,-5},{-8,-2},{-8,0},{-10,3},{-2,2},{-10,4}
   };

   vector<Point*> points;
   for (int j = 0; j < 1/*000000/18*/; j++) {
       for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++) {
           points.push_back(arr + i);
       }
   }
   vector<Point*> result;
   findConvexHull(result, points);
   vector<Point*>::iterator it;
   for (it = result.begin(); it!=result.end(); it++)
      cout << "(" << (*it)->x << ", " <<(*it)->y <<") ";
   cout << points.size() << " " << result.size() << endl;
   //(-9, -5) (-10, 3) (-10, 4) (-7, 8) (8, 6) (8, -7) (6, -10)
}
