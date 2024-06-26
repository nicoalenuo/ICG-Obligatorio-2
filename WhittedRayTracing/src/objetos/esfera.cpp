#include "../../lib/objetos/esfera.h"

bool esfera::interseca(rayo rayo_in, vector_3& punto_interseccion_out) {
	vector_3 Q = rayo_in.get_origen() - posicion;
	float a = rayo_in.get_direccion().producto_interno(rayo_in.get_direccion());
	float b = 2 * rayo_in.get_direccion().producto_interno(Q);
	float c = Q.producto_interno(Q) - radio * radio;
	float d = b * b - 4 * a * c;  

	if (d < 0) {
		return false;
	}
	else {
		float t1 = (-b - sqrt(d)) / 2 * a;
		float t2 = (-b + sqrt(d)) / 2 * a;
		if (t1 >= 0) {
			punto_interseccion_out = rayo_in.get_origen() + rayo_in.get_direccion() * t1;
			return true;
		}
		else if (t2 >= 0) {
			punto_interseccion_out = rayo_in.get_origen() + rayo_in.get_direccion() * t2;
			return true;
		} else {
			return false;
		}
	}
}

vector_3 esfera::normal(vector_3 punto, rayo Rayo) {
	return (punto - posicion).normalizado();
}