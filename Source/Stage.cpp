#include "stage.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cfloat>
#include <deque>
#include <map>
#include <numeric>
#include <tuple>
#include <string>
#include <vector>
#include <utility>

namespace
{
    Stage* activeStage = nullptr;
    const int MAX_MARKS = 12000;
    const int MARK_VERTICES = 16;
    const int MARK_INDICES = 42;
    const int MARKS_PER_BATCH = 3000; // 48000頂点: unsigned short内
    const float PAINT_GRID = 0.12f;
    float Limit(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
    VECTOR Cross(VECTOR a, VECTOR b) { return VGet(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x); }
    struct Triangle { VECTOR p[3], normal, lo, hi; float uv[3][2] = {}; int material=-1; };
    struct Node { VECTOR lo, hi; int left=-1, right=-1, begin=0, end=0; };
    struct Batch { std::vector<VERTEX3D> v; std::vector<unsigned short> indices; };
    using MarkKey = std::tuple<int,int,int,int>;
    struct Mark { int slot=0, bits=0, triangle=0; bool rainbow=false; bool visible=true; std::vector<VECTOR> polygon; VECTOR normal; };
    bool Overlap(VECTOR a,VECTOR b,VECTOR c,VECTOR d)
    { return a.x<=d.x && b.x>=c.x && a.y<=d.y && b.y>=c.y && a.z<=d.z && b.z>=c.z; }
    void Expand(VECTOR& lo, VECTOR& hi, VECTOR p)
    {
        lo=VGet((std::min)(lo.x,p.x),(std::min)(lo.y,p.y),(std::min)(lo.z,p.z));
        hi=VGet((std::max)(hi.x,p.x),(std::max)(hi.y,p.y),(std::max)(hi.z,p.z));
    }
    // 三角形の内部・辺・頂点のうち、点pに最も近い位置を求める。
    VECTOR Closest(VECTOR p,const Triangle& t)
    {
        VECTOR a=t.p[0], b=t.p[1], c=t.p[2], ab=VSub(b,a), ac=VSub(c,a), ap=VSub(p,a);
        float d1=VDot(ab,ap),d2=VDot(ac,ap);
        if(d1<=0 && d2<=0)return a;
        VECTOR bp=VSub(p,b);float d3=VDot(ab,bp),d4=VDot(ac,bp);
        if(d3>=0 && d4<=d3)return b;
        float vc=d1*d4-d3*d2;
        if(vc<=0 && d1>=0 && d3<=0)return VAdd(a,VScale(ab,d1/(d1-d3)));
        VECTOR cp=VSub(p,c);float d5=VDot(ab,cp),d6=VDot(ac,cp);
        if(d6>=0 && d5<=d6)return c;
        float vb=d5*d2-d1*d6;
        if(vb<=0 && d2>=0 && d6<=0)return VAdd(a,VScale(ac,d2/(d2-d6)));
        float va=d3*d6-d5*d4;
        if(va<=0 && d4-d3>=0 && d5-d6>=0)return VAdd(b,VScale(VSub(c,b),(d4-d3)/((d4-d3)+(d5-d6))));
        float inverse=1.0f/(va+vb+vc);
        return VAdd(a,VAdd(VScale(ab,vb*inverse),VScale(ac,vc*inverse)));
    }
    COLOR_U8 PaintColor(VECTOR p,const Mark& mark)
    {
        if(!mark.rainbow)return GetColorU8((mark.bits&1)?245:25,(mark.bits&4)?225:25,(mark.bits&2)?245:25,255);
        const int colors[7][3]={{245,35,45},{255,125,15},{250,230,20},{30,205,65},{10,200,235},{35,75,240},{155,35,225}};
        float t=(0.5f-0.5f*cosf((p.x+p.z*0.45f+p.y*0.3f)/2.4f*DX_PI_F*2))*6;
        int i=(std::min)(5,static_cast<int>(t));float f=t-i;

        return GetColorU8(static_cast<int>(colors[i][0]+(colors[i+1][0]-colors[i][0])*f), static_cast<int>(colors[i][1]+(colors[i+1][1]-colors[i][1])*f), static_cast<int>(colors[i][2]+(colors[i+1][2]-colors[i][2])*f),255);
    }
    // 円形の塗り跡を三角形の内側で切り取る。斜面や端でも宙にはみ出さない。
    std::vector<VECTOR> ClipBrush(const Triangle& t,VECTOR center,float radius)
    {
        VECTOR u=VNorm(VSub(t.p[1],t.p[0])),v=Cross(t.normal,u);
        std::vector<VECTOR> polygon;

        for(int i=0;i<10;++i)
        {
            // DxLibの既存メッシュと同じ頂点順にする。
            float angle=-i*DX_PI_F*2/10;
            polygon.push_back(VAdd(center,VAdd(VScale(u,cosf(angle)*radius),VScale(v,sinf(angle)*radius))));
        }
        for(int e=0;e<3 && !polygon.empty();++e)
        {
            VECTOR a=t.p[e],edge=VSub(t.p[(e+1)%3],a);
            VECTOR inward=Cross(t.normal,edge);
            std::vector<VECTOR> result;
            VECTOR previous=polygon.back();float dp=VDot(VSub(previous,a),inward);
            for(VECTOR current:polygon)
            {
                float dc=VDot(VSub(current,a),inward);
                if((dc>=0)!=(dp>=0))
                {
                    float rate=dp/(dp-dc);
                    result.push_back(VAdd(previous,VScale(VSub(current,previous),rate)));
                }
                if(dc>=0)result.push_back(current);
                previous=current;dp=dc;
            }
            polygon.swap(result);
        }
        return polygon;
    }
}

// ------------------------------------------------------------
// 参照コードの「灰色 -> 元の色」の演出を既存 Stage に追加する。
// Stage.h は元のままで利用できるよう、追加状態は Impl に保持。
// ------------------------------------------------------------
struct RestoreTexture
{
    int original=-1, current=-1, graph=-1;
    int width=0, height=0;
    bool dirty=false;
    std::vector<float> progress;
    std::vector<float> start;
    std::vector<int> active;
};
struct RestoreMaterial
{
    COLOR_F difOriginal={},ambOriginal={},emiOriginal={};
    COLOR_F difGray={},ambGray={},emiGray={};
    int texture=-1;
    float start=-1.0f;
    float progress=0.0f;
};
namespace
{
    constexpr float RESTORE_SECONDS=0.50f;
    constexpr float SPREAD_DELAY=0.03f;
    constexpr int PAINT_PIXEL_RADIUS=12;
    // 子供スライムの赤・青・緑の塗り跡を描画する。
    constexpr bool DRAW_COLOR_MARKS=true;

    float Clamp01(float v) { return (std::max)(0.0f,(std::min)(1.0f,v)); }
    COLOR_F Gray(COLOR_F c)
    {
        float g=Clamp01((c.r*0.299f+c.g*0.587f+c.b*0.114f)*0.70f);
        return GetColorF(g,g,g,c.a);
    }
    COLOR_F LerpColor(COLOR_F a,COLOR_F b,float t)
    {
        return GetColorF(a.r+(b.r-a.r)*t,a.g+(b.g-a.g)*t,a.b+(b.b-a.b)*t,a.a+(b.a-a.a)*t);
    }
    std::string TexturePath(const char* modelPath,const char* imageName)
    {
        if(!imageName || !imageName[0]) return "";
        std::string name(imageName);
        if(name[0]=='/' || name[0]=='\\' || (name.size()>1 && name[1]==':'))return name;
        std::string base=modelPath?modelPath:"";
        size_t slash=base.find_last_of("/\\");
        return (slash==std::string::npos?std::string():base.substr(0,slash+1))+name;
    }
    void ReleaseTexture(RestoreTexture& t)
    {
        if(t.graph>=0)DeleteGraph(t.graph);
        if(t.current>=0)DeleteSoftImage(t.current);
        if(t.original>=0)DeleteSoftImage(t.original);
        t=RestoreTexture{};
    }
}

struct Stage::Impl
{
    int model=-1;
    std::string error;
    std::vector<BoxCollider> emptyBoxes;
    std::vector<Triangle> triangles;
    std::vector<int> order;
    std::vector<Node> nodes;
    std::map<MarkKey,Mark> marks;
    std::deque<MarkKey> markOrder;
    std::vector<Batch> batches;
    std::vector<RestoreTexture> textures;
    std::vector<RestoreMaterial> materials;
    float gameTime=0.0f;

    bool InitTexture(int index,const char* modelPath)
    {
        RestoreTexture& t=textures[index];
        const char* filename=MV1GetTextureName(model,index);
        std::string path=TexturePath(modelPath,filename);
        if(!path.empty())t.original=LoadARGB8ColorSoftImage(path.c_str());
        if(t.original<0 && filename && filename[0])
            t.original=LoadARGB8ColorSoftImage(filename);
        // モデルが元テクスチャを保持していれば、ファイルが無くても取得を試みる。
        if(t.original<0)
        {
            int graph=MV1GetTextureGraphHandle(model,index),w=0,h=0;
            if(graph>=0 && GetGraphSize(graph,&w,&h)>=0 && w>0 && h>0)
            {
                int captured=MakeARGB8ColorSoftImage(w,h);
                int screen=MakeScreen(w,h,TRUE);
                if(captured>=0 && screen>=0)
                {
                    int previous=GetDrawScreen();
                    SetDrawScreen(screen);ClearDrawScreen();
                    DrawGraph(0,0,graph,FALSE);
                    if(GetDrawScreenSoftImage(0,0,w-1,h-1,captured)>=0) t.original=captured;
                    else DeleteSoftImage(captured);
                    SetDrawScreen(previous);
                }
                else if(captured>=0)DeleteSoftImage(captured);
                if(screen>=0)DeleteGraph(screen);
            }
        }
        if(t.original<0)return false;
        GetSoftImageSize(t.original,&t.width,&t.height);
        if(t.width<=0 || t.height<=0){ReleaseTexture(t);return false;}
        t.current=MakeARGB8ColorSoftImage(t.width,t.height);
        if(t.current<0){ReleaseTexture(t);return false;}
        const size_t pixelCount=static_cast<size_t>(t.width)*t.height;
        t.progress.assign(pixelCount,0.0f);
        t.start.assign(pixelCount,-1.0f);
        for(int y=0;y<t.height;++y)
            for(int x=0;x<t.width;++x)
            {
                int r=0,g=0,b=0,a=255;
                GetPixelSoftImage(t.original,x,y,&r,&g,&b,&a);
                int gray=static_cast<int>((r*0.299f+g*0.587f+b*0.114f)*0.70f);
                DrawPixelSoftImage(t.current,x,y,gray,gray,gray,a);
            }
        t.graph=CreateGraphFromSoftImage(t.current);
        if(t.graph<0){ReleaseTexture(t);return false;}
        MV1SetTextureGraphHandle(model,index,t.graph,FALSE);
        return true;
    }

    void InitRestoration(const char* modelPath)
    {
        int count=MV1GetTextureNum(model);
        if(count>0)
        {
            textures.resize(count);
            for(int i=0;i<count;++i)InitTexture(i,modelPath);
        }
        int materialCount=MV1GetMaterialNum(model);
        if(materialCount<0)materialCount=0;
        materials.resize(materialCount);
        for(int i=0;i<materialCount;++i)
        {
            RestoreMaterial& m=materials[i];
            m.difOriginal=MV1GetMaterialDifColor(model,i);
            m.ambOriginal=MV1GetMaterialAmbColor(model,i);
            m.emiOriginal=MV1GetMaterialEmiColor(model,i);
            m.difGray=Gray(m.difOriginal);
            m.ambGray=Gray(m.ambOriginal);
            m.emiGray=Gray(m.emiOriginal);
            int tex=MV1GetMaterialDifMapTexture(model,i);
            if(tex>=0 && tex<static_cast<int>(textures.size()) && textures[tex].graph>=0) m.texture=tex;
            if(m.texture<0)
            {
                MV1SetMaterialDifColor(model,i,m.difGray);
                MV1SetMaterialAmbColor(model,i,m.ambGray);
                MV1SetMaterialEmiColor(model,i,m.emiGray);
            }
        }
    }

    void PaintUV(int textureIndex,float u,float v,float radiusScale)
    {
        if(textureIndex<0 || textureIndex>=static_cast<int>(textures.size()))return;
        RestoreTexture& t=textures[textureIndex];
        if(t.graph<0)return;
        int cx=static_cast<int>(Clamp01(u)*(t.width-1)+0.5f);
        int cy=static_cast<int>(Clamp01(v)*(t.height-1)+0.5f);
        int radius=(std::max)(1,(std::min)(128,static_cast<int>(PAINT_PIXEL_RADIUS*radiusScale)));
        for(int dy=-radius;dy<=radius;++dy)
            for(int dx=-radius;dx<=radius;++dx)
            {
                int x=cx+dx,y=cy+dy;
                if(x<0||y<0||x>=t.width||y>=t.height)continue;
                float distance=sqrtf(static_cast<float>(dx*dx+dy*dy))/radius;
                if(distance>1.0f)continue;
                int pixel=y*t.width+x;
                if(t.progress[pixel]>=1.0f)continue;
                float startTime=gameTime+distance*SPREAD_DELAY;
                if(t.start[pixel]<0)
                {
                    t.start[pixel]=startTime;
                    t.active.push_back(pixel);
                }
                else t.start[pixel]=(std::min)(t.start[pixel],startTime);
            }
    }

    void RestoreAt(int triangleId,VECTOR point,float brushRadius)
    {
        const Triangle& tri=triangles[triangleId];
        int index=tri.material;
        if(index<0 || index>=static_cast<int>(materials.size()))return;
        RestoreMaterial& mat=materials[index];
        if(mat.texture<0)
        {
            if(mat.start<0)mat.start=gameTime;
            return;
        }
        VECTOR ab=VSub(tri.p[1],tri.p[0]);
        VECTOR ac=VSub(tri.p[2],tri.p[0]);
        VECTOR ap=VSub(point,tri.p[0]);
        float d00=VDot(ab,ab),d01=VDot(ab,ac),d11=VDot(ac,ac);
        float d20=VDot(ap,ab),d21=VDot(ap,ac);
        float denom=d00*d11-d01*d01;
        if(fabsf(denom)<1.e-10f)return;
        float b=(d11*d20-d01*d21)/denom;
        float c=(d00*d21-d01*d20)/denom;
        float a=1.0f-b-c;
        if(a<0||b<0||c<0)return;
        float u=a*tri.uv[0][0]+b*tri.uv[1][0]+c*tri.uv[2][0];
        float v=a*tri.uv[0][1]+b*tri.uv[1][1]+c*tri.uv[2][1];
        // 元コードの画素ブラシと同じ処理。半径のみ 3D ブラシに連動。
        PaintUV(mat.texture,u,v,(std::max)(0.3f,brushRadius/0.3f));
    }

    void UpdateRestoration(float delta)
    {
        gameTime+=(std::min)(0.1f,(std::max)(0.0f,delta));
        for(size_t ti=0;ti<textures.size();++ti)
        {
            RestoreTexture& t=textures[ti];
            if(t.graph<0)continue;
            for(int i=static_cast<int>(t.active.size())-1;i>=0;--i)
            {
                int pixel=t.active[i];
                if(gameTime<t.start[pixel])continue;
                float rate=Clamp01((gameTime-t.start[pixel])/RESTORE_SECONDS);
                float smooth=rate*rate*(3.0f-2.0f*rate);
                if(smooth>t.progress[pixel])
                {
                    t.progress[pixel]=smooth;
                    int x=pixel%t.width,y=pixel/t.width,r=0,g=0,b=0,a=255;
                    GetPixelSoftImage(t.original,x,y,&r,&g,&b,&a);
                    float gray=(r*0.299f+g*0.587f+b*0.114f)*0.70f;
                    DrawPixelSoftImage(t.current,x,y,
                        static_cast<int>(gray+(r-gray)*smooth),
                        static_cast<int>(gray+(g-gray)*smooth),
                        static_cast<int>(gray+(b-gray)*smooth),a);
                    t.dirty=true;
                }
                if(rate>=1.0f){t.active[i]=t.active.back();t.active.pop_back();}
            }
            if(t.dirty)
            {
                MV1SetTextureGraphHandle(model,static_cast<int>(ti),-1,FALSE);
                if(t.graph>=0)DeleteGraph(t.graph);
                t.graph=CreateGraphFromSoftImage(t.current);
                if(t.graph>=0)MV1SetTextureGraphHandle(model,static_cast<int>(ti),t.graph,FALSE);
                t.dirty=false;
            }
        }
        for(size_t i=0;i<materials.size();++i)
        {
            RestoreMaterial& m=materials[i];
            if(m.texture>=0 || m.start<0 || m.progress>=1.0f)continue;
            float t=Clamp01((gameTime-m.start)/RESTORE_SECONDS);
            float smooth=t*t*(3.0f-2.0f*t);
            m.progress=t;
            MV1SetMaterialDifColor(model,static_cast<int>(i),LerpColor(m.difGray,m.difOriginal,smooth));
            MV1SetMaterialAmbColor(model,static_cast<int>(i),LerpColor(m.ambGray,m.ambOriginal,smooth));
            MV1SetMaterialEmiColor(model,static_cast<int>(i),LerpColor(m.emiGray,m.emiOriginal,smooth));
        }
    }


    int Build(int begin,int end)
    {
        Node n;n.begin=begin;n.end=end;n.lo=VGet(FLT_MAX,FLT_MAX,FLT_MAX);n.hi=VGet(-FLT_MAX,-FLT_MAX,-FLT_MAX);
        for(int i=begin;i<end;++i){Expand(n.lo,n.hi,triangles[order[i]].lo);Expand(n.lo,n.hi,triangles[order[i]].hi);}
        int index=static_cast<int>(nodes.size());nodes.push_back(n);
        if(end-begin>8)
        {
            VECTOR size=VSub(n.hi,n.lo);int axis=size.x>size.z?0:2;if(size.y>(axis==0?size.x:size.z))axis=1;
            int middle=(begin+end)/2;
            std::nth_element(order.begin()+begin,order.begin()+middle,order.begin()+end,[&](int a,int b)
            {
                VECTOR x=VAdd(triangles[a].lo,triangles[a].hi),y=VAdd(triangles[b].lo,triangles[b].hi);
                return axis==0?x.x<y.x:axis==1?x.y<y.y:x.z<y.z;
            });
            int left=Build(begin,middle),right=Build(middle,end);
            nodes[index].left=left;nodes[index].right=right;
        }
        return index;
    }
    template<class F> void Query(int index,VECTOR lo,VECTOR hi,const F& visit) const
    {
        if(index<0 || nodes.empty())return;
        const Node& n=nodes[index];if(!Overlap(lo,hi,n.lo,n.hi))return;
        if(n.left>=0){Query(n.left,lo,hi,visit);Query(n.right,lo,hi,visit);}
        else for(int i=n.begin;i<n.end;++i){int id=order[i];if(Overlap(lo,hi,triangles[id].lo,triangles[id].hi))visit(id);}
    }
    void WriteMark(const Mark& mark)
    {
        int batchIndex=mark.slot/MARKS_PER_BATCH;
        if(static_cast<int>(batches.size())<=batchIndex)batches.resize(batchIndex+1);
        Batch& b=batches[batchIndex];int offset=mark.slot%MARKS_PER_BATCH;
        int base=offset*MARK_VERTICES,indexBase=offset*MARK_INDICES;
        if(static_cast<int>(b.v.size())<base+MARK_VERTICES)
        {b.v.resize(base+MARK_VERTICES);b.indices.resize(indexBase+MARK_INDICES);}
        int count=mark.visible?static_cast<int>(mark.polygon.size()):0;
        for(int i=0;i<MARK_VERTICES;++i)
        {
            VECTOR p=count>0?mark.polygon[i<count?i:0]:VGet(0,0,0);VERTEX3D vertex={};
            vertex.pos=VAdd(p,VScale(mark.normal,0.016f));vertex.norm=mark.normal;vertex.dif=PaintColor(p,mark);
            b.v[base+i]=vertex;
        }
        for(int i=0;i<MARK_INDICES;++i)b.indices[indexBase+i]=static_cast<unsigned short>(base);
        for(int i=1;i<count-1;++i)
        {
            b.indices[indexBase+(i-1)*3]=static_cast<unsigned short>(base);
            b.indices[indexBase+(i-1)*3+1]=static_cast<unsigned short>(base+i);
            b.indices[indexBase+(i-1)*3+2]=static_cast<unsigned short>(base+i+1);
        }
    }
};
Stage::Stage():impl(new Impl()){}
Stage::~Stage(){Finalize();}
Stage* Stage::Active(){return activeStage;}

void Stage::Finalize()
{
    if(activeStage==this)activeStage=nullptr;
    if(impl->model>=0)
    {
        for(size_t i=0;i<impl->textures.size();++i)
        {
            if(impl->textures[i].graph>=0)
            {
                MV1SetTextureGraphHandle(impl->model,static_cast<int>(i),-1,FALSE);
            }
        }
    }
    for(RestoreTexture& t:impl->textures)ReleaseTexture(t);
    impl->textures.clear();impl->materials.clear();impl->gameTime=0.0f;
    if(impl->model>=0)MV1DeleteModel(impl->model);
    impl->model=-1;impl->triangles.clear();impl->nodes.clear();impl->order.clear();
    impl->marks.clear();impl->markOrder.clear();impl->batches.clear();
}
bool Stage::Initialize(const char* path,float scale,VECTOR position)
{
    Finalize();impl->error.clear();
    if(!path || !std::isfinite(scale) || scale<=0 || !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
    {impl->error="stage error";return false;}
    impl->model=MV1LoadModel(path);
    if(impl->model<0){impl->error=std::string("ロード出来てない: ")+path;return false;}
    MV1SetScale(impl->model,VGet(scale,scale,scale));MV1SetPosition(impl->model,position);
    if(MV1SetupReferenceMesh(impl->model,-1,TRUE)<0)
    {impl->error="model error.";Finalize();return false;}
    MV1_REF_POLYGONLIST mesh=MV1GetReferenceMesh(impl->model,-1,TRUE);
    bool valid=mesh.Vertexs && mesh.Polygons && mesh.PolygonNum>0;
    for(int i=0;valid && i<mesh.PolygonNum;++i)
    {
        Triangle t;t.material=mesh.Polygons[i].MaterialIndex;t.lo=VGet(FLT_MAX,FLT_MAX,FLT_MAX);t.hi=VGet(-FLT_MAX,-FLT_MAX,-FLT_MAX);
        for(int k=0;k<3;++k)
        {
            int v=mesh.Polygons[i].VIndex[k];if(v<0 || v>=mesh.VertexNum){valid=false;break;}
            t.p[k]=mesh.Vertexs[v].Position;
            t.uv[k][0]=mesh.Vertexs[v].TexCoord[0].u;
            t.uv[k][1]=mesh.Vertexs[v].TexCoord[0].v;
            if(!std::isfinite(t.p[k].x)||!std::isfinite(t.p[k].y)||!std::isfinite(t.p[k].z)){valid=false;break;}
            Expand(t.lo,t.hi,t.p[k]);
        }
        if(!valid)break;

        VECTOR cross=Cross(VSub(t.p[1],t.p[0]),VSub(t.p[2],t.p[0]));
        if(VSquareSize(cross)<1.0e-12f)continue;
        t.normal=VNorm(cross);impl->triangles.push_back(t);
    }
    MV1TerminateReferenceMesh(impl->model,-1,TRUE);

    if(!valid || impl->triangles.empty()){impl->error="Stage_error";Finalize();return false;}

    impl->order.resize(impl->triangles.size());std::iota(impl->order.begin(),impl->order.end(),0);
    impl->Build(0,static_cast<int>(impl->order.size()));
    impl->InitRestoration(path);
    activeStage=this;

    return true;
}
const std::string& Stage::GetError()const{return impl->error;}
const std::vector<BoxCollider>& Stage::GetBoxes()const{return impl->emptyBoxes;}

bool Stage::FindFloor(float x,float z,float minY,float maxY,float& y,VECTOR* normal)const
{
    bool found=false;float best=minY;VECTOR bestNormal=VGet(0,1,0);
    impl->Query(0,VGet(x-0.0001f,minY,z-0.0001f),VGet(x+0.0001f,maxY,z+0.0001f),[&](int id)
    {
        const Triangle& t=impl->triangles[id];if(fabsf(t.normal.y)<0.55f)return;
        VECTOR a=t.p[0],b=t.p[1],c=t.p[2];
        float det=(b.z-c.z)*(a.x-c.x)+(c.x-b.x)*(a.z-c.z);if(fabsf(det)<1.0e-10f)return;
        float wa=((b.z-c.z)*(x-c.x)+(c.x-b.x)*(z-c.z))/det;
        float wb=((c.z-a.z)*(x-c.x)+(a.x-c.x)*(z-c.z))/det;float wc=1-wa-wb;
        if(wa<-0.0001f || wb<-0.0001f || wc<-0.0001f)return;
        float height=wa*a.y+wb*b.y+wc*c.y;
        if(height<minY || height>maxY || (found && height<=best))return;
        best=height;found=true;bestNormal=t.normal.y<0?VScale(t.normal,-1):t.normal;
    });
    if(found){y=best;if(normal)*normal=bestNormal;}return found;
}
bool Stage::BabyCanStand(float x,float z,float currentY,float radius,float& y)const
{
    if(!FindFloor(x,z,currentY-0.35f,currentY+0.12f,y))return false;
    const float offsets[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
    for(const auto& offset:offsets)
    {
        float edgeY;
        if(!FindFloor(x+offset[0]*radius,z+offset[1]*radius,y-radius*0.8f,y+radius*0.8f,edgeY))return false;
    }
    // 胴体が壁にめり込む移動先を除外する。床の斜面は床検索で確認済み。
    VECTOR center=VGet(x,y+radius*0.55f,z);bool blocked=false;
    impl->Query(0,VGet(x-radius,y+0.015f,z-radius),VGet(x+radius,y+radius*1.1f,z+radius),[&](int id)
    {
        const Triangle& t=impl->triangles[id];
        if(fabsf(t.normal.y)>=0.55f)return;
        VECTOR q=Closest(center,t);
        if(q.y>y+0.01f && VSquareSize(VSub(center,q))<radius*radius)blocked=true;
    });
    return !blocked;
}
void Stage::ResolveSphere(VECTOR& p,VECTOR& velocity,float radius,bool& grounded,float& impact,VECTOR& wall,float& wallDepth)const
{
    impact=0;wallDepth=0;wall=VGet(0,0,0);
    for(int pass=0;pass<4;++pass)
    {
        VECTOR d=VGet(radius,radius,radius),lo=VSub(p,d),hi=VAdd(p,d);
        impl->Query(0,lo,hi,[&](int id)
        {
            const Triangle& t=impl->triangles[id];VECTOR q=Closest(p,t),delta=VSub(p,q);float length2=VSquareSize(delta);
            if(length2>(radius+0.0001f)*(radius+0.0001f))return;
            float distance=sqrtf(length2);VECTOR n;
            if(distance>0.000001f)n=VScale(delta,1.0f/distance);
            else {n=t.normal;if(VDot(velocity,n)>0)n=VScale(n,-1);}

            float depth=(std::max)(0.0f,radius-distance);p=VAdd(p,VScale(n,depth));
            float vn=VDot(velocity,n);

            if(vn<0){if(n.y>0.55f)impact=(std::max)(impact,-vn);velocity=VSub(velocity,VScale(n,vn));}
            if(n.y>0.55f)grounded=true;
            if(fabsf(n.y)<0.45f && depth>wallDepth){wallDepth=depth;wall=VScale(n,-1);}
        });
    }
}
void Stage::Paint(VECTOR position,float contactRadius,float brushRadius,int color)
{
    if(impl->model<0 || brushRadius<=0 || contactRadius<0 || color<0 || color>4)return;
    float reach=contactRadius+0.04f;
    float queryRadius=sqrtf(reach*reach+brushRadius*brushRadius);
    VECTOR d=VGet(queryRadius,queryRadius,queryRadius);
    impl->Query(0,VSub(position,d),VAdd(position,d),[&](int id)
    {
        const Triangle& triangle=impl->triangles[id];
        VECTOR closest=Closest(position,triangle);
        if(VSquareSize(VSub(position,closest))>queryRadius*queryRadius)return;
        float signedDistance=VDot(VSub(position,triangle.p[0]),triangle.normal);
        if(fabsf(signedDistance)>reach)return;
        VECTOR projected=VSub(position,VScale(triangle.normal,signedDistance));
        VECTOR contact=Closest(projected,triangle);

        // color==4: 本体のLキーで接触したステージの元色を復元する。
        // 子供のRGB塗装とは混ぜない。
        if(color==4)
        {
            impl->RestoreAt(id,contact,brushRadius);
            // 既に付着したペンキのうち、接触場所の近くを消す。
            // メッシュのスロットは退避せず、ゼロ面積の三角形で非表示にする。
            for(auto& entry : impl->marks)
            {
                Mark& mark=entry.second;
                if(!mark.visible || mark.triangle!=id || mark.polygon.empty())continue;
                VECTOR center=VGet(0,0,0);
                for(const VECTOR& point : mark.polygon)center=VAdd(center,point);
                center=VScale(center,1.0f/static_cast<float>(mark.polygon.size()));
                if(VSquareSize(VSub(contact,center))<=brushRadius*brushRadius)
                {
                    mark.visible=false;
                    impl->WriteMark(mark);
                }
            }
            return;
        }
        // color=0 虹色 / color=1 赤 / color=2 青 / color=3 緑（元色は復元しない）。
        if(!DRAW_COLOR_MARKS)return;
        MarkKey key=std::make_tuple(id,static_cast<int>(floorf(projected.x/PAINT_GRID)),  static_cast<int>(floorf(projected.y/PAINT_GRID)),static_cast<int>(floorf(projected.z/PAINT_GRID)));
        auto found=impl->marks.find(key);
        if(found!=impl->marks.end())
        {
            Mark& mark=found->second;
            int bits=(color==0)?mark.bits:(mark.visible?(mark.bits|(1<<(color-1))):(1<<(color-1)));
            if(bits!=mark.bits || !mark.visible || (color==0 && !mark.rainbow) || (color!=0 && mark.rainbow))
            {
                mark.bits=bits;
                mark.visible=true;
                mark.rainbow=(color==0);
                impl->WriteMark(mark);
            }
            return;
        }
        Mark mark;mark.polygon=ClipBrush(triangle,projected,brushRadius);
        if(mark.polygon.size()<3 || mark.polygon.size()>MARK_VERTICES)return;
        mark.normal=signedDistance>=0?triangle.normal:VScale(triangle.normal,-1);
        if(signedDistance<0)std::reverse(mark.polygon.begin(),mark.polygon.end());
        mark.slot=static_cast<int>(impl->marks.size());
        if(impl->marks.size()>=MAX_MARKS)
        {
            auto oldest=impl->marks.find(impl->markOrder.front());mark.slot=oldest->second.slot;
            impl->marks.erase(oldest);impl->markOrder.pop_front();
        }
        mark.bits=(color==0)?0:(1<<(color-1));mark.rainbow=(color==0);mark.triangle=id;
        impl->marks.emplace(key,mark);impl->markOrder.push_back(key);impl->WriteMark(mark);
    });
}
void Stage::Update(const SlimeModel& slime)
{
    // 既存シグネチャを維持。呼び出しごとに60fps想定で更新。
    // 可変フレームレートにするなら Stage.h 側に deltaTime 引数を追加する。
    impl->UpdateRestoration(1.0f/60.0f);
    if(CheckHitKey(KEY_INPUT_K))
        Paint(slime.GetPosition(),slime.GetRadius(),slime.GetRadius()*0.65f,0);
    if(CheckHitKey(KEY_INPUT_L))
        Paint(slime.GetPosition(),slime.GetRadius(),slime.GetRadius()*0.65f,4);
    if(CheckHitKey(KEY_INPUT_L))
        Paint(slime.GetPosition(),slime.GetRadius(),slime.GetRadius()*0.65f,4);
}
void Stage::Draw()
{
    if(impl->model<0)return;
    MV1DrawModel(impl->model);DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND,0);
    for (Batch& b : impl->batches) { DrawPolygonIndexed3D(b.v.data(), static_cast<int>(b.v.size()), b.indices.data(), static_cast<int>(b.indices.size() / 3), DX_NONE_GRAPH, TRUE); }
}
