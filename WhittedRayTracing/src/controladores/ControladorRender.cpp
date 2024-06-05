#include "../../lib/controladores/ControladorRender.h"

color negro( 0., 0., 0. );
color blanco( 1., 1., 1. );
color gris( 0.3, 0.3, 0.3 );
color ambiente = gris;

ControladorRender* ControladorRender::instancia = nullptr;

ControladorRender::ControladorRender() {

}

ControladorRender::~ControladorRender() {

}

ControladorRender* ControladorRender::getInstance() {
	if (instancia == nullptr)
		instancia = new ControladorRender();
	return instancia;
}

color get_componente_ambiente(objeto* objeto) {
	return ambiente * objeto->get_color_difuso() * objeto->get_coeficiente_ambiente();
}

/*
Se manejan dos situaciones:
- Cuando el rayo est� dentro del objeto
- Cuando el rayo est� afuera.

Si el rayo est� afuera, necesitas hacer cosi positivo cosi = -N.I

Si el rayo est� dentro, necesitas invertir los �ndices de refracci�n y negar la normal N
*/
vector_3 direccion_refractada(vector_3 I, vector_3 N, float ior) {
	float cosi = min(1.f, max(-1.f, I.producto_interno(N)));
	
	float etai = COEFICIENTE_REFRACCION_VACIO; // Medio antes de la refraccion
	float etat = ior;						   // Medio despues de la refraccion
	vector_3 n = N;
	
	if (cosi < 0) { //El rayo esta afuera
		cosi = -cosi;
	} else { //El rayo esta dentro
		n = -n;
		etai = ior;
		etat = COEFICIENTE_REFRACCION_VACIO;
	}

	float eta = etai / etat;

	float k = 1 - eta * eta * (1 - cosi * cosi);

	return k < 0 ? vector_3(0.f, 0.f, 0.f) : (I * eta) + n * (eta * cosi - sqrtf(k));
}

color get_componente_difuso(objeto* objeto, vector_3 N, vector_3 L_i) {
	//k_d * O_d_alpha * (N * L_i)
	return 
		objeto->get_color_difuso() *      // O_d_alpha
		objeto->get_coeficiente_difuso() * // k_d
		N.producto_interno(L_i);
}

vector_3 direccion_reflejada(vector_3 L, vector_3 N) {
	return L - N * 2 * (L.producto_interno(N));
}

color get_componente_especular(objeto* objeto, vector_3 V, vector_3 N, vector_3 L) {
	return
		objeto->get_color_especular() *    //O_s_alpha
		objeto->get_coeficiente_especular() * //k_s
		pow(direccion_reflejada(L, N).producto_interno(V), EXPONENTE_ESPECULAR); // (R_i * V)^n
}

color get_componente_luz(objeto* objeto, rayo Rayo, vector_3 punto_interseca, vector_3 normal) {
	color color_resultado = negro;

	vector<luz*> luces = ControladorEscena::getInstance()->get_luces();

	for (size_t i = 0; i < luces.size(); i++) {

		rayo rayo_sombra = rayo(punto_interseca + normal * EPSILON,
			luces[i]->get_posicion() - (punto_interseca + normal * EPSILON));
		
		if (rayo_sombra.get_direccion().producto_interno(normal) > 0) {

			float distancia_de_la_luz = (luces[i]->get_posicion() - punto_interseca).norma();

			// f_att_i
			float atenuacion = min(1 / (c1 + c2 * distancia_de_la_luz + c3 * distancia_de_la_luz * distancia_de_la_luz), 1.f);

			// S_i
			float sombra = ControladorEscena::getInstance()->obtener_cantidad_de_luz_bloqueada(rayo_sombra, luces[i]->get_posicion());

			// k_d * O_d_alpha * (N * L_i)
			color componente_difuso = get_componente_difuso(objeto, normal, rayo_sombra.get_direccion());

			// ks * O_s_alpha * (R_i * V)^n
			color componente_especular = get_componente_especular(objeto, -Rayo.get_direccion(), normal, rayo_sombra.get_direccion());

			color_resultado = color_resultado +
				luces[i]->get_color() * // I_p_alpha
				sombra * atenuacion *   // S_i * f_att_i
				(componente_difuso + componente_especular);
		}
	}

	return color_resultado;
}

color ControladorRender::get_componente_reflectivo(objeto* objeto, rayo rayo_r, int profundidad) {
	return
		traza_rr(rayo_r, profundidad + 1) *
		objeto->get_coeficiente_especular();
}

color ControladorRender::get_componente_refractivo(objeto* objeto, rayo rayo_t, int profundidad) {
	return
		traza_rr(rayo_t, profundidad + 1) *
		objeto->get_coeficiente_transmicion();
}


color ControladorRender::sombra_rr(objeto* objeto, rayo Rayo, vector_3 punto_interseca, vector_3 normal, int profundidad) {

	//color = termino del ambiente;
	color color_resultado = get_componente_ambiente(objeto);

	//for (cada luz)
	color_resultado = color_resultado + get_componente_luz(objeto, Rayo, punto_interseca, normal);
	
	if (profundidad < PROFUNDIDAD_MAXIMA) {
		//if (objeto es reflejante)
		if (objeto->get_es_reflectante()) {
			vector_3 dir_reflejada = direccion_reflejada(Rayo.get_direccion(), normal);
			rayo rayo_r = rayo(punto_interseca + dir_reflejada * EPSILON,
				dir_reflejada
			);
			color_resultado = color_resultado + get_componente_reflectivo(objeto, rayo_r, profundidad);
		}
		//if (objeto es transparente)
		if (objeto->get_coeficiente_transmicion() > 0) {
			//if (no ocurre la reflexi�n interna total) { -> coeficiente de refraccionn < 1
			vector_3 dir_refractada = direccion_refractada(Rayo.get_direccion(), normal, objeto->get_coeficiente_transmicion());
			rayo rayo_t = rayo(punto_interseca + dir_refractada * EPSILON,
				dir_refractada
			);
			color_resultado = color_resultado + get_componente_refractivo(objeto, rayo_t, profundidad);
		}
	}
	return color_resultado;
}

color ControladorRender::traza_rr(rayo Rayo, int profundidad) {
	objeto* objeto;
	vector_3 punto_interseca;

	
	if (ControladorEscena::getInstance()->obtener_objeto_intersecado_mas_cercano(Rayo, objeto, punto_interseca)) {
		
		vector_3 normal = objeto->normal(punto_interseca, Rayo);
		return sombra_rr(objeto, Rayo, punto_interseca, normal, profundidad);
	}
	else {
		return negro;
	}

}

imagen* ControladorRender::whitted_ray_tracing() {
	camara* camara = ControladorEscena::getInstance()->get_camara();
	vector_3 origen = camara->getPosicionCamara(); //ojo de la camara
	vector_3 plano = vector_3(0.f, 0.f, 0.f);

	int imagen_width = camara->get_imagen_width();
	int imagen_height = camara->get_imagen_height();

	imagen* img_resultado = new imagen(imagen_width, imagen_height);

	#pragma omp parallel for collapse(2) //para paralelizacion
	for (int i = 0; i < imagen_height; i++) {
		for (int j = 0; j < imagen_width; j++) {
			rayo Rayo = rayo(origen,
				vector_3((float)(j - imagen_width / 2), (float)(i - imagen_height / 2), plano.get_z()) - origen);

			img_resultado->set_pixel(i, j, traza_rr(Rayo, 1));
		}
	}

	return img_resultado;
}

