/* Tracy-2

   J. Bengtsson, CBP, LBL      1990 - 1994   Pascal version
                 SLS, PSI      1995 - 1997
   M. Boege      SLS, PSI      1998          C translation
   L. Nadolski   SOLEIL        2002          Link to NAFF, Radia field maps
   J. Bengtsson  NSLS-II, BNL  2004 -


   To generate a lattice flat file.

   Type codes:

     marker     -1
     drift	 0
     multipole   1
     cavity      2
     thin kick   3
     wiggler     4
     kick_map    6

   Integration methods:

     fourth order symplectic integrator   4

   Format:

     name, family no, kid no, element no
     type code, integration method, no of integration steps
     apertures: xmin, xmax, ymin, ymax

   The following lines follows depending on element type.

     type

     drift:	 L

     multipole:  hor., ver. displacement, roll angle (design),
                                          roll angle (error)
                 L, 1/rho, entrance angle, exit angle
                 apertures[4]
		 no of nonzero multipole coeff., n design
		 n, b , a
		     n   n
		    ...

     wiggler:    L [m], lambda [m]
                 no of harmonics
                 harm no, kxV [1/m], BoBrhoV [1/m], kxH, BoBrhoH, phi
                    ...

     cavity:	 cavity voltage/beam energy [eV], omega/c, beam energy [eV]

     thin kick:	 hor., ver. displacement, roll angle (total)
		 no of nonzero multipole coeff.
		 n, b , a
		     n   n
		    ...

     kick_map:   order <file name>

*/


#define snamelen 10

// numerical type codes
#define marker_   -1
#define drift_     0
#define mpole_     1
#define cavity_    2
#define thinkick_  3
#define wiggler_   4
#define kick_map_  6


void prtName(FILE *fp, const int i,
	     const int type, const int method, const int N)
{
  fprintf(fp, "%-15s %4d %4d %4d\n",
	  Cell[i].Elem->name, Cell[i].Fnum, Cell[i].Knum, i);
  fprintf(fp, " %3d %3d %3d\n", type, method, N);
  fprintf(fp, " %23.16e %23.16e %23.16e %23.16e\n",
	  Cell[i].maxampl[X_][0], Cell[i].maxampl[X_][1],
	  Cell[i].maxampl[Y_][0], Cell[i].maxampl[Y_][1]);
}


void prtHOM(FILE *fp, const int n_design, const mpolArray bn, const int Order)
{
  int i, nmpole;

  nmpole = 0;
  for (i = 1; i <= Order; i++)
    if ((bn[HOMmax-i] != 0e0) || (bn[HOMmax+i] != 0e0)) nmpole++;
  fprintf(fp, "  %2d %2d\n", nmpole, n_design);
  for (i = 1; i <= Order; i++) {
    if ((bn[HOMmax-i] != 0e0) || (bn[HOMmax+i] != 0e0))
      fprintf(fp, "%3d %23.16e %23.16e\n", i, bn[HOMmax+i], bn[HOMmax-i]);
  }
}


void prtmfile(const char mfile_dat[])
{
  int           i, j;
  MpoleType     *M;
  CavityType    *C;
  WigglerType   *W;
  InsertionType *ID;
  FILE          *mfile;

  mfile = file_write(mfile_dat);
  for (i = 0; i <= globval.Cell_nLoc; i++) {
    switch (Cell[i].Elem->kind) {
    case marker:
      prtName(mfile, i, marker_, 0, 0);
      break;
    case drift:
      prtName(mfile, i, drift_, 0, 0);
      fprintf(mfile, " %23.16e\n", Cell[i].Elem->L);
      break;
    case Mpole:
      if (Cell[i].Elem->L != 0e0) {
	M = static_cast<MpoleType*>(Cell[i].Elem);
	prtName(mfile, i, mpole_, M->method, M->n);
	fprintf(mfile, " %23.16e %23.16e %23.16e %23.16e\n",
		Cell[i].dS[X_], Cell[i].dS[Y_],
		M->rollpar,
		M->rollsys
		+M->rollrms*M->rollrnd);
	fprintf(mfile, " %23.16e %23.16e %23.16e %23.16e %23.16e\n",
		Cell[i].Elem->L, M->irho, M->tx1, M->tx2, M->gap);
	prtHOM(mfile, M->n_design, M->bn, M->order);
      } else {
	prtName(mfile, i, thinkick_, M->method,
		M->n);
	fprintf(mfile, " %23.16e %23.16e %23.16e\n",
		Cell[i].dS[X_], Cell[i].dS[Y_],
		M->rollsys+M->rollrms*M->rollrnd);
	prtHOM(mfile, M->n_design, M->bn, M->order);
      }
      break;
    case Cavity:
      C = static_cast<CavityType*>(Cell[i].Elem);
      prtName(mfile, i, cavity_, 0, 0);
      fprintf(mfile, " %23.16e %23.16e %d %23.16e\n",
	      C->volt/(1e9*globval.Energy), 2.0*M_PI*C->freq/c0, C->h,
	      1e9*globval.Energy);
      break;
    case Wigl:
      W = static_cast<WigglerType*>(Cell[i].Elem);
      prtName(mfile, i, wiggler_, W->method, W->n);
      fprintf(mfile, " %23.16e %23.16e\n", Cell[i].Elem->L, W->lambda);
      fprintf(mfile, "%2d\n", W->n_harm);
      for (j = 0; j < W->n_harm; j++) {
	fprintf(mfile, "%2d %23.16e %23.16e %23.16e %23.16e %23.16e\n",
		W->harm[j], W->kxV[j], W->BoBrhoV[j], W->kxH[j], W->BoBrhoH[j],
		W->phi[j]);
      }
      break;
    case Insertion:
      ID = static_cast<InsertionType*>(Cell[i].Elem);
      prtName(mfile, i, kick_map_, ID->method, ID->n);
      if (ID->firstorder)
	fprintf(mfile, " %3.1lf %1d %s\n", ID->scaling, 1, ID->fname1);
      else
	fprintf(mfile, " %3.1lf %1d %s\n", ID->scaling, 2, ID->fname2);
      break;
    default:
      printf("prtmfile: unknown type %d\n", Cell[i].Elem->kind);
      exit(1);
      break;
    }
  }
  fclose(mfile);
}
