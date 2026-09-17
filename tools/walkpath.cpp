// Offline development-route search using the Xbox's actual streaming/collision.
// This is a host test tool, not client movement or server position injection.
extern "C" {
#include "wx_region.h"
}
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <map>
#include <vector>
#include <algorithm>
static unsigned allocated;
extern "C" unsigned wx_free_memory(void){return 44u*1024u*1024u-allocated;}
extern "C" void* wx_gpu_alloc(unsigned n){auto p=(unsigned*)std::malloc(n+4);if(!p)return nullptr;*p=n;allocated+=n;return p+1;}
extern "C" void wx_gpu_free(void* p){if(p){auto q=(unsigned*)p-1;allocated-=*q;std::free(q);}}
struct Node{std::array<float,3> p;int x,y,parent;float cost;};
int main(int argc,char** argv){
    if(argc!=10){std::fprintf(stderr,"Usage: walkpath world.wxi map start_x start_y start_z goal_x goal_y goal_z margin\n");return 2;}
    std::array<float,3> start,goal;for(unsigned k=0;k<3;k++){start[k]=std::strtof(argv[k+3],nullptr);goal[k]=std::strtof(argv[k+6],nullptr);if(!std::isfinite(start[k])||!std::isfinite(goal[k]))return 2;}
    float margin=std::strtof(argv[9],nullptr);if(!std::isfinite(margin)||margin<2||margin>100)return 2;
    unsigned map=(unsigned)std::strtoul(argv[2],nullptr,10);WxRegion region;WxScene scene;wx_scene_init(&scene);
    if(!wx_region_open(&region,argv[1]))return 1;
    auto stream=[&](const std::array<float,3>& p,unsigned n){for(unsigned i=0;i<n;i++){wx_region_update(&region,&scene,map,p.data());wx_stream(&scene,p.data());}};
    auto heuristic=[&](const std::array<float,3>& p){return std::hypot(goal[0]-p[0],goal[1]-p[1])+std::fabs(goal[2]-p[2]);};
    float lo[2],hi[2];for(unsigned k=0;k<2;k++){lo[k]=std::min(start[k],goal[k])-margin;hi[k]=std::max(start[k],goal[k])+margin;}
    std::vector<Node> nodes;nodes.reserve(32768);nodes.push_back({start,0,0,-1,0});
    using Key=std::array<int,3>;std::map<Key,float> best;best[{0,0,(int)std::lround(start[2]*2)}]=0;
    using Queue=std::pair<float,unsigned>;std::priority_queue<Queue,std::vector<Queue>,std::greater<Queue>> open;open.push({heuristic(start),0});
    stream(start,160);int found=-1;unsigned expanded=0,pending=0,blocked=0,closest=0;
    while(!open.empty()&&nodes.size()<32768){
        unsigned current=open.top().second;open.pop();Node n=nodes[current];
        if(heuristic(n.p)<heuristic(nodes[closest].p))closest=current;
        if(n.cost>best[{n.x,n.y,(int)std::lround(n.p[2]*2)}]+.001f)continue;
        if(std::hypot(goal[0]-n.p[0],goal[1]-n.p[1])<.4f&&std::fabs(goal[2]-n.p[2])<.8f){found=(int)current;break;}
        stream(n.p,4);expanded++;
        for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++)if(x||y){
            std::array<float,3> p={start[0]+(n.x+x)*.5f,start[1]+(n.y+y)*.5f,n.p[2]};
            if(p[0]<lo[0]||p[0]>hi[0]||p[1]<lo[1]||p[1]>hi[1])continue;
            int ok=wx_walk(&scene,n.p.data(),p[0],p[1],&p[2]);
            if(!ok&&(wx_walk_blocker(nullptr)==2||wx_walk_blocker(nullptr)==3)){stream(n.p,160);pending++;ok=wx_walk(&scene,n.p.data(),p[0],p[1],&p[2]);}
            if(!ok){blocked++;continue;}
            float cost=n.cost+std::hypot(x*.5f,y*.5f)+std::fabs(p[2]-n.p[2]);Key key={n.x+x,n.y+y,(int)std::lround(p[2]*2)};
            auto old=best.find(key);if(old!=best.end()&&cost>=old->second-.001f)continue;best[key]=cost;
            nodes.push_back({p,n.x+x,n.y+y,(int)current,cost});open.push({cost+heuristic(p),(unsigned)nodes.size()-1});
        }
    }
    if(found>=0){std::vector<unsigned> path;for(int i=found;i>=0;i=nodes[i].parent)path.push_back((unsigned)i);std::reverse(path.begin(),path.end());
        auto segment=[&](const std::array<float,3>& a,const std::array<float,3>& b){
            float length=std::hypot(b[0]-a[0],b[1]-a[1]);if(length<.001f)return true;
            unsigned steps=std::max(1u,(unsigned)std::ceil(length/.15f));
            // Leave room for controller steering/float quantization; a line
            // touching the edge of a wall is not a usable replay route.
            for(int lane=-1;lane<=1;lane++){
                float sx=-(b[1]-a[1])/length*.12f*lane,sy=(b[0]-a[0])/length*.12f*lane;
                auto p=a;stream(p,160);float z=p[2];if(!wx_walk(&scene,p.data(),p[0]+sx,p[1]+sy,&z))return false;
                p={p[0]+sx,p[1]+sy,z};auto from=p;
                for(unsigned i=1;i<=steps;i++){stream(p,4);float x=from[0]+(b[0]-a[0])*i/steps,y=from[1]+(b[1]-a[1])*i/steps;z=p[2];
                    if(!wx_walk(&scene,p.data(),x,y,&z))return false;p={x,y,z};}
                if(std::fabs(p[2]-b[2])>=.3f)return false;
            }return true;
        };
        for(unsigned i=0;i<path.size();){auto p=nodes[path[i]].p;std::printf("%.6f %.6f %.6f\n",p[0],p[1],p[2]);
            if(i+1==path.size())break;unsigned next=(unsigned)path.size()-1;while(next>i+1&&!segment(p,nodes[path[next]].p))next--;i=next;}
    }
    std::fprintf(stderr,"found=%d expanded=%u states=%zu pending=%u blocked=%u loads=%u failures=%u\n",found>=0,expanded,nodes.size(),pending,blocked,scene.loads,scene.failures+region.failures);
    std::fprintf(stderr,"closest=%.4f,%.4f,%.4f distance=%.4f\n",nodes[closest].p[0],nodes[closest].p[1],nodes[closest].p[2],heuristic(nodes[closest].p));
    int good=found>=0&&!scene.failures&&!region.failures;wx_pack_close(&scene);wx_region_close(&region);return good&&!allocated?0:1;
}
