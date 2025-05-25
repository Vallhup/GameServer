#pragma once

class Mesh;
struct Vertex;

void CreateENGLetter_S(Mesh& letter);
vector<Vertex> LetterSVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterSIndices();

void CreateENGLetter_C(Mesh& letter);
vector<Vertex> LetterCVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterCIndices();

void CreateENGLetter_L(Mesh& letter);
vector<Vertex> LetterLVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterLIndices();

void CreateENGLetter_A(Mesh& letter);
vector<Vertex> LetterAVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterAIndices();

void CreateENGLetter_R(Mesh& letter);
vector<Vertex> LetterRVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterRIndices();

void CreateENGLetter_O(Mesh& letter);
vector<Vertex> LetterOVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterOIndices();

void CreateENGLetter_T(Mesh& letter);
vector<Vertex> LetterTVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterTIndices();

void CreateENGLetter_U(Mesh& letter);
vector<Vertex> LetterUVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterUIndices();

void CreateENGLetter_N(Mesh& letter);
vector<Vertex> LetterNVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterNIndices();

void CreateENGLetter_E(Mesh& letter);
vector<Vertex> LetterEVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterEIndices();

void CreateENGLetter_M(Mesh& letter);
vector<Vertex> LetterMVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterMIndices();

void CreateENGLetter_D(Mesh& letter);
vector<Vertex> LetterDVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterDIndices();

void CreateENGLetter_Y(Mesh& letter);
vector<Vertex> LetterYVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterYIndices();

void CreateENGLetter_W(Mesh& letter);
vector<Vertex> LetterWVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterWIndices();

void CreateKORLetter_HO(Mesh& letter);
vector<Vertex> LetterHOVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterHOIndices();

void CreateKORLetter_JEONG(Mesh& letter);
vector<Vertex> LetterJEONGVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterJEONGIndices();

void CreateKORLetter_LEE(Mesh& letter);
vector<Vertex> LetterLEEVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterLEEIndices();

void CreateKORLetter_GE(Mesh& letter);
vector<Vertex> LetterGEVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterGEIndices();

void CreateKORLetter_IM(Mesh& letter);
vector<Vertex> LetterIMVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterIMIndices();

void CreateKORLetter_PEU(Mesh& letter);
vector<Vertex> LetterPEUVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterPEUIndices();

void CreateKORLetter_LO(Mesh& letter);
vector<Vertex> LetterLOVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterLOIndices();

void CreateKORLetter_GEU(Mesh& letter);
vector<Vertex> LetterGEUVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterGEUIndices();

void CreateKORLetter_RAE(Mesh& letter);
vector<Vertex> LetterRAEVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterRAEIndices();

void CreateKORLetter_MING(Mesh& letter);
vector<Vertex> LetterMINGVertices(const float R, const float G, const float B, const float A);
vector<UINT> LetterMINGIndices();

void CreateNUMLetter_1(Mesh& letter);
vector<Vertex> NumberOneVertices(const float R, const float G, const float B, const float A);
vector<UINT> NumberOneIndices();

void CreateNUMLetter_2(Mesh& letter);
vector<Vertex> NumberTwoVertices(const float R, const float G, const float B, const float A);
vector<UINT> NumberTwoIndices();

void CreateNUMLetter_3(Mesh& letter);
vector<Vertex> NumberThreeVertices(const float R, const float G, const float B, const float A);
vector<UINT> NumberThreeIndices();

void CreateExclamMark(Mesh& letter);
vector<Vertex> ExclamMarkVertices(const float R, const float G, const float B, const float A);
vector<UINT> ExclamMarkIndices();

void CreatePlaneIndexes(Mesh& letter);
vector<Vertex> PlaneVertices(const float R, const float G, const float B, const float A);
vector<UINT> PlaneIndices();

void CreateCubeIndexes(Mesh& letter);
vector<Vertex> CubeVertices(const float R, const float G, const float B, const float A);
vector<UINT> CubeIndices();

vector<Vertex> ExplosionCubeVertices();
vector<UINT> ExplosionCubeIndices();

void CreateRCCubeIndexes(Mesh& letter);
vector<Vertex> RCCubeVertices();
vector<UINT> RCCubeIndices();

void CreateTankBodyIndexes(Mesh& letter);
vector<Vertex> TankBodyVertices();
vector<UINT> TankBodyIndices();