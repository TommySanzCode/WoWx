// Initial bounded walking collision. Uses terrain plus original WMO/M2 collision
// triangles; swimming, moving platforms and a full swept capsule remain future work.
#include "wx_runtime.h"
#include <math.h>
static float dot(const float* a,const float* b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static void cross(const float* a,const float* b,float* out){out[0]=a[1]*b[2]-a[2]*b[1];out[1]=a[2]*b[0]-a[0]*b[2];out[2]=a[0]*b[1]-a[1]*b[0];}
static int nearby(const WxEntry* e,float x,float y,float margin){float dx=e->center[0]-x,dy=e->center[1]-y;float r=e->radius+margin;return dx*dx+dy*dy<=r*r;}
static unsigned blocked;static int blocked_entry=-1;
unsigned wx_walk_blocker(int* entry){if(entry)*entry=blocked_entry;return blocked;}
int wx_floor(WxScene* s,float x,float y,float top,float bottom,float* z){
    if(!s||!z||!isfinite(x)||!isfinite(y)||!isfinite(top)||!isfinite(bottom)||top<bottom)return 0;
    int found=0;float best=bottom;
    for(int k=0;k<WX_CACHE_SLOTS;k++){
        WxResident* r=&s->slots[k];if(r->entry<0)continue;const WxEntry* e=&s->entries[r->entry];
        if((e->kind!=WX_KIND_TERRAIN&&e->kind!=WX_KIND_COLLISION)||!nearby(e,x,y,0))continue;
        for(unsigned i=0;i<e->index_count;i+=3){
            const float* a=r->vertices[r->indices[i]].p;const float* b=r->vertices[r->indices[i+1]].p;const float* c=r->vertices[r->indices[i+2]].p;
            // The barycentric edge tolerance accepts points slightly outside a
            // triangle. Expand these cheap XY bounds to retain those hits.
            unsigned axis=0;for(;axis<2;axis++){
                float lo=a[axis]<b[axis]?a[axis]:b[axis],hi=a[axis]>b[axis]?a[axis]:b[axis];
                if(c[axis]<lo)lo=c[axis];if(c[axis]>hi)hi=c[axis];
                float at=axis?y:x,margin=.0002f*(hi-lo)+.01f;
                if(at<lo-margin||at>hi+margin)break;
            }
            if(axis<2)continue;
            float ab[3]={b[0]-a[0],b[1]-a[1],b[2]-a[2]},ac[3]={c[0]-a[0],c[1]-a[1],c[2]-a[2]},normal[3];cross(ab,ac,normal);
            if(normal[2]*normal[2]<.42f*dot(normal,normal))continue;
            float det=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1]);if(fabsf(det)<.00001f)continue;
            float u=((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(y-c[1]))/det;
            float v=((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(y-c[1]))/det;
            if(u<-.0001f||v<-.0001f||u+v>1.0001f)continue;
            float height=u*a[2]+v*b[2]+(1-u-v)*c[2];
            if(height<=top&&height>=best){best=height;found=1;}
        }
    }
    if(found)*z=best;return found;
}
static float triangle_fraction(const float* start,const float* delta,const float* a,const float* b,const float* c){
    float ab[3]={b[0]-a[0],b[1]-a[1],b[2]-a[2]},ac[3]={c[0]-a[0],c[1]-a[1],c[2]-a[2]};
    float p[3];cross(delta,ac,p);float det=dot(ab,p);if(fabsf(det)<.000001f)return 2;
    float t[3]={start[0]-a[0],start[1]-a[1],start[2]-a[2]},q[3];cross(t,ab,q);
    float u=dot(t,p)/det,v=dot(delta,q)/det,distance=dot(ac,q)/det;
    return u>=0&&v>=0&&u+v<=1&&distance>.00001f&&distance<=1?distance:2;
}
float wx_camera_distance(WxScene* s,const float* focus,const float* forward,const float* right,const float* up,float desired){
    if(!s||!focus||!forward||!right||!up||!isfinite(desired)||desired<0||desired>12)return 0;
    for(unsigned k=0;k<3;k++)if(!isfinite(focus[k])||!isfinite(forward[k])||!isfinite(right[k])||!isfinite(up[k]))return 0;
    if(fabsf(dot(forward,forward)-1)>.01f||fabsf(dot(right,right)-1)>.01f||fabsf(dot(up,up)-1)>.01f)return 0;
    float delta[3],middle[3],lo[3],hi[3],best=1;
    for(unsigned k=0;k<3;k++){
        delta[k]=-forward[k]*desired;middle[k]=focus[k]+delta[k]*.5f;
        float end=focus[k]+delta[k],margin=.4f*fabsf(right[k])+.3f*fabsf(up[k]);
        lo[k]=(focus[k]<end?focus[k]:end)-margin;hi[k]=(focus[k]>end?focus[k]:end)+margin;
    }
    for(unsigned i=0;i<s->header.count;i++){
        const WxEntry* e=&s->entries[i];if(e->kind!=WX_KIND_COLLISION&&e->kind!=WX_KIND_TERRAIN)continue;
        float d[3]={e->center[0]-middle[0],e->center[1]-middle[1],e->center[2]-middle[2]},radius=e->radius+desired*.5f+.5f;
        if(dot(d,d)>radius*radius)continue;
        const WxResident* r=NULL;
        for(unsigned j=0;j<WX_CACHE_SLOTS;j++)if(s->slots[j].entry>=0){
            const WxEntry* loaded=&s->entries[s->slots[j].entry];
            if(s->slots[j].entry==(int)i||((e->flags&WX_PLACEMENT_ID)&&(loaded->flags&WX_PLACEMENT_ID)&&loaded->kind==e->kind&&loaded->id==e->id&&loaded->reserved[0]==e->reserved[0])){r=&s->slots[j];break;}}
        if(!r)return 0; // Collision still streaming: pull the camera to the player.
        for(unsigned triangle=0;triangle<e->index_count;triangle+=3){
            const float* a=r->vertices[r->indices[triangle]].p;const float* b=r->vertices[r->indices[triangle+1]].p;const float* c=r->vertices[r->indices[triangle+2]].p;
            // Reject triangles outside the complete five-ray swept box before
            // doing ray/triangle arithmetic (most of a building lies outside it).
            unsigned axis=0;for(;axis<3;axis++)if((a[axis]<lo[axis]&&b[axis]<lo[axis]&&c[axis]<lo[axis])||
                (a[axis]>hi[axis]&&b[axis]>hi[axis]&&c[axis]>hi[axis]))break;
            if(axis<3)continue;
            // Cover the camera's near-plane corners as well as its center.
            for(unsigned ray=0;ray<5;ray++){
                float start[3];for(unsigned k=0;k<3;k++)start[k]=focus[k]+(ray?right[k]*(ray&1?.4f:-.4f)+up[k]*(ray&2?.3f:-.3f):0);
                float t=triangle_fraction(start,delta,a,b,c);if(t<best)best=t;
            }
        }
    }
    float distance=best*desired-(best<1?.2f:0);return distance>0?distance:0;
}
int wx_walk(WxScene* s,const float* from,float x,float y,float* z){
    blocked=1;blocked_entry=-1;
    if(!s||!from||!z||!isfinite(from[0])||!isfinite(from[1])||!isfinite(from[2])||!isfinite(x)||!isfinite(y))return 0;
    float dx=x-from[0],dy=y-from[1],length=sqrtf(dx*dx+dy*dy);if(length>1)return 0;
    // Refuse to cross geometry whose collision data has not arrived yet.
    for(unsigned i=0;i<s->header.count;i++)if(s->entries[i].kind==WX_KIND_COLLISION&&nearby(&s->entries[i],x,y,1)){
        int resident=0;const WxEntry* required=&s->entries[i];
        for(int j=0;j<WX_CACHE_SLOTS;j++)if(s->slots[j].entry>=0){
            const WxEntry* loaded=&s->entries[s->slots[j].entry];
            if(s->slots[j].entry==(int)i||((required->flags&WX_PLACEMENT_ID)&&(loaded->flags&WX_PLACEMENT_ID)&&loaded->kind==WX_KIND_COLLISION&&loaded->id==required->id&&loaded->reserved[0]==required->reserved[0])){resident=1;break;}}
        if(!resident){blocked=2;blocked_entry=(int)i;return 0;}
    }
    float floor;if(!wx_floor(s,x,y,from[2]+.65f,from[2]-3,&floor)){blocked=3;return 0;}
    if(length>.00001f){
        float side[2]={-dy/length*.3f,dx/length*.3f};
        float delta[3]={dx,dy,floor-from[2]};
        float lo[3],hi[3];
        for(unsigned axis=0;axis<3;axis++){
            float end=from[axis]+delta[axis];
            lo[axis]=from[axis]<end?from[axis]:end;hi[axis]=from[axis]>end?from[axis]:end;
            if(axis<2){lo[axis]-=fabsf(side[axis])+.01f;hi[axis]+=fabsf(side[axis])+.01f;}
            else {lo[axis]+=.69f;hi[axis]+=1.61f;}
        }
        for(int k=0;k<WX_CACHE_SLOTS;k++){
            WxResident* r=&s->slots[k];if(r->entry<0)continue;const WxEntry* e=&s->entries[r->entry];
            if(e->kind!=WX_KIND_COLLISION||!nearby(e,x,y,2))continue;
            for(unsigned i=0;i<e->index_count;i+=3){
                const float* a=r->vertices[r->indices[i]].p;const float* b=r->vertices[r->indices[i+1]].p;const float* c=r->vertices[r->indices[i+2]].p;
                // All six body rays lie in this swept box. Avoid ray tests for
                // unrelated walls, roofs and distant parts of a large building.
                unsigned axis=0;for(;axis<3;axis++)if((a[axis]<lo[axis]&&b[axis]<lo[axis]&&c[axis]<lo[axis])||
                    (a[axis]>hi[axis]&&b[axis]>hi[axis]&&c[axis]>hi[axis]))break;
                if(axis<3)continue;
                for(int height=0;height<2;height++)for(int lateral=-1;lateral<=1;lateral++){
                    float start[3]={from[0]+side[0]*lateral,from[1]+side[1]*lateral,from[2]+(height?1.6f:.7f)};
                    if(triangle_fraction(start,delta,a,b,c)<=1){blocked=4;blocked_entry=r->entry;return 0;}
                }
            }
        }
    }
    blocked=0;*z=floor;return 1;
}
