#include <algorithm>
#include <stack>
#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
using namespace std;

typedef long double real;

const real epsilon = (real)1e-15;
const real PI2 = (real)M_PI / 2;

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
    if (dir > -epsilon && dir < epsilon) {
        return dist2(point0, point2) >= dist2(point0, point1) + epsilon;
    }
    return dir < 0;
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

    for (int i = 0; i < n; i++) {
        if (i >= 3) {
            while (s.size() >= 2 && direction(extractSecond(s), s.top(), points[i]) > epsilon) {
                s.pop();
            }
        }

        if (i == 0 || dist2(points[i], points[i - 1]) > epsilon) {
            s.push(points[i]);
        }
    }

    while (!s.empty()) {
        res.push_back(s.top());
        s.pop();
    }
}

void outerTangle(vector<Point*> &points, vector<Circle*> &circles) {
    int size = circles.size();
    for (vector<Circle*>::iterator i = circles.begin(); i != circles.end(); i++) {
        for (vector<Circle*>::iterator j = i + 1; j != circles.end(); j++) {
            const real x1 = (*i)->x;
            const real y1 = (*i)->y;
            const real r1 = (*i)->r;
            const real x2 = (*j)->x;
            const real y2 = (*j)->y;
            const real r2 = (*j)->r;
            const real dx = x2 - x1;
            const real dy = y2 - y1;
            const real dr = r2 - r1;
            const real d = sqrt(dx * dx + dy * dy);
            if (d < epsilon || abs(dr) > d + epsilon) {
                continue;
            }
            const real beta1 = dr > d - epsilon ? PI2 : dr < -d + epsilon ? -PI2 : asin(dr / d);
            const real gamma = -atan2(dy, dx);
            for (int sign = -1; sign <= 1; sign += 2) {
                const real beta = beta1 * sign;
                const real alpha = gamma - beta;
                const real sinAlpha = sin(alpha) * sign;
                const real cosAlpha = cos(alpha) * sign;

                Point* p1 = new Point();
                p1 -> x = x1 + r1 * sinAlpha;
                p1 -> y = y1 + r1 * cosAlpha;
                points.push_back(p1);

                Point* p2 = new Point();
                p2 -> x = x2 + r2 * sinAlpha;
                p2 -> y = y2 + r2 * cosAlpha;
                points.push_back(p2);
            }
        }
    }
}

int main() {
    /*Point arr[] = {
        //{0,0}, {1,0}, {10,0}, {3,0}, {-3,0}, {5,0}
        {-7,8},{-4,6},{2,6},{6,4},{8,6},{7,-2},{4,-6},{8,-7},{0,0},
        {3,-2},{6,-10},{0,-6},{-9,-5},{-8,-2},{-8,0},{-10,3},{-2,2},{-10,4},
        //{-7,8},{-4,6},{2,6},{6,4},{8,6},{7,-2},{4,-6},{8,-7},{0,0},
        //{3,-2},{6,-10},{0,-6},{-9,-5},{-8,-2},{-8,0},{-10,3},{-2,2},{-10,4}
    };

    vector<Point*> points;
    for (int j = 0; j < 1000000/18; j++) {
        for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++) {
            points.push_back(arr + i);
        }
    }
    vector<Point*> result;
    findConvexHull(result, points);
    vector<Point*>::iterator it;
    for (it = result.begin(); it!=result.end(); it++)
        cout << "(" << (*it)->x << ", " << (*it)->y <<") ";
    cout << points.size() << " " << result.size() << endl;
    //(-9, -5) (-10, 3) (-10, 4) (-7, 8) (8, 6) (8, -7) (6, -10)
    */

    Circle arr[] = {
        {0, 1, 0},
        {0, 0, 1},
    };

    vector<Circle*> circles;
    for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++) {
        circles.push_back(arr + i);
    }

    vector<Point*> points;
    outerTangle(points, circles);
    vector<Point*>::iterator it;
    for (it = points.begin(); it!=points.end(); it++)
        cout << "(" << (*it)->x << ", " << (*it)->y <<") ";
}
