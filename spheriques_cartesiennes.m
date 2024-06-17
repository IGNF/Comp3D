format long

function [C,S]=arc(Xa,Ya,Xb,Yb)
%Za = sqrt(1-Xa**2-Ya**2);
%Zb = sqrt(1-Xb**2-Yb**2);
%C  = Xa*Xb+Ya*Yb+Za*Zb;
%S  = sqrt((Ya*Zb-Yb*Za)**2+(Xa*Zb-Xb*Za)**2+(Xa*Yb-Xb*Ya)**2);
tmp=sqrt((Xb-Xa)**2+(Yb-Ya)**2);
C=cos(tmp);
S=sin(tmp);
end;

function [gis]=gisement(X1,Y1,X2,Y2)
DX = X2 - X1;
DY = Y2 - Y1;
DXY = sqrt(DX**2 + DY**2);
gis=  2 * atan(DX / (DY + DXY));
end;

centerx=0
centery=0
radius=6378351.7230885401368
gamma=0


Xm = 5000
Ym = 0
Zm = 5000

facteur_stereo=1+(Xm**2+Ym**2)/(4*radius**2)


[C,S]=arc(0,0,Xm/radius,Ym/radius);
alpha_normal = atan2(S,C)
alpha=Xm/radius
tmp=sqrt((Xm/radius)**2+(Ym/radius)**2);
C=cos(tmp);
S=sin(tmp);
alpha = atan2(S,C)


test_est=alpha*radius

V =  gisement(0, 0 , Xm, Ym);
Az = V + gamma

VX = sin(alpha) * sin(Az) * radius;
VY = sin(alpha) * cos(Az) * radius;
VZ = (cos(alpha) - 1) * radius;

VunitaireX = S * sin(V);
VunitaireY = S * cos(V);
VunitaireZ = C;

Xc = VX + Zm * VunitaireX+centerx;
Yc = VY + Zm * VunitaireY+centery;
Zc = VZ + Zm * VunitaireZ;

Xc
Yc
Zc

%Xc=5003.9195079051
%Yc =  0
%Zc = 4998.0387094923

dist_earth_center=sqrt(Xc*Xc+Yc*Yc+(Zc+radius)*(Zc+radius))

h=dist_earth_center-radius

VunitaireX=Xc/dist_earth_center
VunitaireY=Yc/dist_earth_center
VunitaireZ=(Zc+radius)/dist_earth_center

X0=Xc-h*VunitaireX
Y0=Yc-h*VunitaireY
Z0=Zc-h*VunitaireZ

dist_earth_center0=sqrt(X0*X0+Y0*Y0+(Z0+radius)*(Z0+radius))-radius

alpha=atan2(sqrt(X0*X0+Y0*Y0),(Z0+radius))
[C,S]=arc(0,0,X0/radius,Y0/radius);
alpha_normal = atan2(S,C)

Az=1.570796326794896558



e=alpha*radius*sin(Az)+centerx
n=alpha*radius*cos(Az)+centery


