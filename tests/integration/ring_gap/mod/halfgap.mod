: ggap.mod
: This is a conductance based gap junction to allow setting g = 0
NEURON {
	POINT_PROCESS HalfGap
	RANGE g, i, vgap
	ELECTRODE_CURRENT i
}
PARAMETER { g = 0 (1/megohm) }
ASSIGNED {
	v (millivolt)
	vgap (millivolt)
	i (nanoamp)
}
BREAKPOINT { i = (vgap - v)*g }
