int is_neighbor_(nc1,nc2,MX,MY,MZ)
int	*nc1, *nc2, *MX, *MY, *MZ;
{
	int	ind1=*nc1-1, ind2=*nc2-1;
	int	mx=*MX, my=*MY, mz=*MZ;
	int	xc, yc, zc;
	int	neighbor, i, j, k;

	xc = ind1 % mx;
	yc = (ind1 / mx) % my;
	zc = ind1 / (mx * my);

	for(i=xc-1; i <= xc+1 ;i++) {
	    if (i < 0 || i >= mx) continue;
	    for(j=yc-1; j <= yc+1 ;j++) {
		if (j < 0 || j >= my) continue;
		for(k=zc-1; k <= zc+1 ;k++) {
		    if (k < 0 || k >= mz) continue;
		    neighbor = i + mx * (j + k * my);
		    if (neighbor == ind1) continue;
		    if (neighbor == ind2) return(1);
		}
	    }
	}
	return(0);
}

void manh_distance_(nc1,nc2)
int	*nc1, *nc2;
{
}

int is_neighbor__(nc1,nc2,MX,MY,MZ)
int *nc1, *nc2, *MX, *MY, *MZ;
{
  return is_neighbor_(nc1,nc2,MX,MY,MZ);
}
