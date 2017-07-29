module rcd {
	sequence<double> Vector;

	interface ParameterUpdateIce {
		void update(int varId1, int varId2, ["cpp:array"] Vector update1, 
					["cpp:array"] Vector update2, bool async);
					
		void init(int varId1);							 				
	};
	
	interface ParameterReadIce {
		void getNodeInput(int varId, out string codedInput);
		void getNodeStaticInput(int varId, out string codedInput);
	};
};
