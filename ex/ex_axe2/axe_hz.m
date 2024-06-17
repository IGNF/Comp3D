%calcul de l'axe horizontal du téléscope
% variables : npt = nombre de points, npos = nombre de positions
% 1 mesure = 1 point du téléscope dans 1 position
% on ne tient pas compte du fait que les points ont été mesurés dans une même position
% on dit juste que chacun des points a tourné autour d'un même axe

% paramètres :
% axe de rotation (pour tout l) :
% x=l*a+x0; y=l*b+y0; z=l*c+z0;
% l'axe est presque horizontal, et pas mal en x, on fixe donc x0, et a = 1
x0=56.63490
% x=l*1+x0; y=l*b+y0; z=l*c+z0;

% paramètre pour chaque point i :
% - li (où est le projeté perpendiculaire du point sur l'axe)
% - ri (distance entre le point et l'axe)

% pas d'inconnue pour chaque position (on ne gère pas les angles)


% observation pour le point i à la position j :
% f(params,obs) = (li*1+1-xij)^2 + (li*b+y0-yij)^2 + (li*c+z0-zij)^2 - ri^2 = 0
%On a aussi une autre obs : les cercles doivent être orthogonaux à l'axe :
% xx'+yy'+zz'=0   : g(params,obs) = (li*1)*(li*1+1-xij) + (li*b)*(li*b+y0-yij) + (li*c)*(li*c+z0-zij) = 0

% (modèle de Gauss-Helmert)


%obs(a,b,c,x0,y0,z0,li,ri,xij,yij,zij):=(li*a+x0-xij)**2 + (li*b+y0-yij)**2 + (li*c+z0-zij)**2 - ri**2;
%diff(obs(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),a);
%diff(obs(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),b);
%diff(obs(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),c);
%diff(obs(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),x0);
%diff(obs(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),y0);
%diff(obs(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),z0);
%diff(obs(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),li);
%diff(obs(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),ri);
%diff(obs(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),xij);
%diff(obs(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),yij);
%diff(obs(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),zij);

%obs2(a,b,c,x0,y0,z0,li,ri,xij,yij,zij):=(li*1)*(li*1+1-xij) + (li*b)*(li*b+y0-yij) + (li*c)*(li*c+z0-zij) = 0
%diff(obs2(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),a);
%diff(obs2(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),b);
%diff(obs2(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),c);
%diff(obs2(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),x0);
%diff(obs2(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),y0);
%diff(obs2(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),z0);
%diff(obs2(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),li);
%diff(obs2(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),ri);
%diff(obs2(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),xij);
%diff(obs2(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),yij);
%diff(obs2(a,b,c,x0,y0,z0,li,ri,xij,yij,zij),zij);


% on a donc 4+2*npt inconnues et nobs*2 relations.

%Paramètres : X=[b c y0 z0 l1 r1 l2 r2 l3 r3 ...]

format long

npt=3 %nombre de prismes
npos=4 %nombre de positions
nparam=4+2*npt %nombre d'inconnues
X=ones(nparam,1); %vecteur des inconnues

X(1,1)=0.5;
X(2,1)=0.2;
X(3,1)=29; 
X(4,1)=15;
for i = 1:npt
  X(4+(i-1)*2+1,1)=1;
  X(4+(i-1)*2+2,1)=5;
end



%observ : les coordonnées des points mesurés : pt=i, pos=j, xij (m), yij, zij, sigmax (m), sigmay (m), sigmaz (m)
observ=[
[ 1, 1,  49.54620 30.36991 10.77295 0.0000013 0.0000014 0.0000014 ],
[ 2, 1,  52.79557 34.74989 16.18197 0.0000014 0.0000012 0.0000015 ],
[ 3, 1,  57.76179 28.50450 11.78516 0.0000013 0.0000015 0.0000013 ],
[ 1, 2,  48.97760 30.66591 12.07635 0.0000011 0.0000009 0.0000015 ],
[ 2, 2,  52.70764 34.04929 17.87631 0.0000011 0.0000011 0.0000014 ],
[ 3, 2,  57.33796 29.28057 11.64578 0.0000012 0.0000014 0.0000009 ],
[ 1, 3,  48.69013 30.68196 13.00251 0.0000011 0.0000010 0.0000015 ],
[ 2, 3,  52.79756 33.38606 18.90310 0.0000011 0.0000012 0.0000013 ],
[ 3, 3,  57.02369 29.78795 11.67859 0.0000013 0.0000014 0.0000009 ],
[ 1, 4,  48.48647 30.55177 13.94178 0.0000012 0.0000012 0.0000015 ],
[ 2, 4,  53.00364 32.58872 19.81086 0.0000013 0.0000014 0.0000013 ],
[ 3, 4,  56.69508 30.26930 11.81126 0.0000013 0.0000014 0.0000013 ]]


nobs=size(observ,1)

%Gt=df/dl et dg/dl  (li*1)*(li*1+1-xij) + (li*b)*(li*b+y0-yij) + (li*c)*(li*c+z0-zij)
%df_dxij= -2 * (li*a+x0-xij)
%df_dyij= -2 * (li*b+y0-yij)
%df_dzij= -2 * (li*c+1-zij)
%dg_dxij= -(li*1)
%dg_dyij= -(li*b)
%dg_dzij= -(li*c)

Gt=zeros(nobs*2,nobs*3);

%A=df/dx et dg/dx
%df_db = 2 * li * (li*b+y0-yij)
%df_dc = 2 * li * (li*c-zij)
%df_dy0= 2 * (li*b+y0-yij)
%df_dz0= 2 * (li*c+z0-zij)
%df_dli= 2 * a * (li*a+x0-xij) + 2 * b * (li*b+y0-yij) + 2 * c * (li*c+1-zij)
%df_dri= -2 * ri
%dg_db = 2*li*li*b+li*(y0-yij)
%dg_dc = 2*li*li*c+li*(z0-zij)
%dg_dy0= li*b
%dg_dz0= li*c
%dg_dli= 2*li+1-xij+2*b*b*li+b*(y0-yij)+2*c*c*li+c*(z0-zij)
%dg_dri= 0
A=zeros(nobs*2,nparam);

%residus
w=zeros(nobs*2,1);
X
Qll=zeros(nobs*3,nobs*3);
P=zeros(nobs*3,nobs*3);

resume_sigma0=[0];
for iter=1:100
  iter
  Gt=zeros(nobs*2,nobs*3);
  A=zeros(nobs*2,nparam);
  w=zeros(nobs*2,1);

  for obs=1:nobs
    pt=observ(obs,1);
    pos=observ(obs,2);
  
    a=1;
    b=X(1,1);
    c=X(2,1);
    %x0=x0;
    y0=X(3,1);
    z0=X(4,1);
    li=X(4+(pt-1)*2+1,1);
    ri=X(4+(pt-1)*2+2,1);
    xij=observ(obs,3);
    yij=observ(obs,4);
    zij=observ(obs,5);
    %partie f
    Gt((obs-1)*2+1,(obs-1)*3+1)=  -2*(li*a+x0-xij); %df/dxij avec i = pt, j = pos
    Gt((obs-1)*2+1,(obs-1)*3+2)=  -2*(li*b+y0-yij); %df/dyij avec i = pt, j = pos
    Gt((obs-1)*2+1,(obs-1)*3+3)=  -2*(li*c+z0-zij); %df/dzij avec i = pt, j = pos
    %partie g
    Gt((obs-1)*2+2,(obs-1)*3+1)=  -(li*a); %dg/dxij avec i = pt, j = pos
    Gt((obs-1)*2+2,(obs-1)*3+2)=  -(li*b); %dg/dyij avec i = pt, j = pos
    Gt((obs-1)*2+2,(obs-1)*3+3)=  -(li*c); %dg/dzij avec i = pt, j = pos

    %partie f
    A((obs-1)*2+1,1)=2 * li * (li*b+y0-yij);
    A((obs-1)*2+1,2)=2 * li * (li*c+z0-zij);
    A((obs-1)*2+1,3)=2 * (li*b+y0-yij);
    A((obs-1)*2+1,4)=2 * (li*c+z0-zij);
    A((obs-1)*2+1,4+(pt-1)*2+1)=2 * a * (li*a+x0-xij) + 2 * b * (li*b+y0-yij) + 2 * c * (li*c+z0-zij);
    A((obs-1)*2+1,4+(pt-1)*2+2)=-2 * ri;
    %partie g
    A((obs-1)*2+2,1)=2*li*li*b+li*(y0-yij);
    A((obs-1)*2+2,2)=2*li*li*c+li*(z0-zij);
    A((obs-1)*2+2,3)=li*b;
    A((obs-1)*2+2,4)=li*c;
    A((obs-1)*2+2,4+(pt-1)*2+1)=2*a*a*li+a*(x0-xij)+2*b*b*li+b*(y0-yij)+2*c*c*li+c*(z0-zij);
    A((obs-1)*2+2,4+(pt-1)*2+2)=0;

    w((obs-1)*2+1,1)=(li*a+x0-xij)**2 + (li*b+y0-yij)**2 + (li*c+z0-zij)**2 - ri**2;
    w((obs-1)*2+2,1)=(li*a)*(li*a+x0-xij) + (li*b)*(li*b+y0-yij) + (li*c)*(li*c+z0-zij);

    Qll((obs-1)*3+1,(obs-1)*3+1)=(observ(obs,6))**2;
    Qll((obs-1)*3+2,(obs-1)*3+2)=(observ(obs,7))**2;
    Qll((obs-1)*3+3,(obs-1)*3+3)=(observ(obs,8))**2;
    P((obs-1)*3+1,(obs-1)*3+1)=1/Qll((obs-1)*3+1,(obs-1)*3+1);
    P((obs-1)*3+2,(obs-1)*3+2)=1/Qll((obs-1)*3+2,(obs-1)*3+2);
    P((obs-1)*3+3,(obs-1)*3+3)=1/Qll((obs-1)*3+3,(obs-1)*3+3);
  end
  
  w
  norm(w);

  Gt;
  A;
  G=Gt';
  S=Qll*G*inv(Gt*Qll*G);
  A_etoile=-S*A;
  B_etoile=S*w;
  dX=inv(A_etoile'*P*A_etoile)*A_etoile'*P*B_etoile
  v=B_etoile-A_etoile*dX
  resume_sigma0(iter)=sqrt((v'*P*v)/(nobs*2-nparam));
  erreur=w-Gt*v+A*dX;
  Qxx=inv(A_etoile'*P*A_etoile);
  

  X=X+dX

end
Qxx
resume_sigma0

%resultats :
b=  X(1)
c=  X(2)
y0= X(3)
z0= X(4)


%sigma0=3.1165557604116

sigmay0=sqrt(Qxx(3,3))*resume_sigma0(iter)
sigmaz0=sqrt(Qxx(4,4))*resume_sigma0(iter)




%calcul du point central entre les deux axes
%axe vertical pris dans axe_vert7_VLBI15.m
Vx=-241.400240929870
Vy=179.290997434313
l=Vx-x0
dy=y0+b*l -Vy

%axe hz : y=bx+d (d=-x0*b+y0)
d=-b*x0+y0;
%perpendiculaire hz à l'axe hz : y=mx+p
m=-1/b;
p=Vy+Vx/b;
%intersection entre ces deux droites
Ax=(p-d)/(b+1/b);
Ay=b*Ax+d;
%le point central est le milieu de V et A
Cx=(Vx+Ax)/2
Cy=(Vy+Ay)/2
l=Cx-x0;
Cz=  c*l+z0
%
%z_nivel=1272.6340

