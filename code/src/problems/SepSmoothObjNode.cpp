#include "problems/SepSmoothObjNode.h"
#include "core/Platform.h"
#include "environments/NetConfig.h"

using namespace Eigen;

const MatrixXd *SmoothObjCache::updateEntry(int id1, int id2, const MatrixXd *matrix) {
	sortIds(id1, id2);
	auto it = data.find(id1);
	assert(it != data.end());
	auto it2 = it->second.find(id2);
	assert(it2 != it->second.end());

	const MatrixXd *p = __sync_val_compare_and_swap(&it2->second, 0, matrix); //TODO: Wrap in platform
	
	if(p) { //entry already exists
		delete matrix;
		matrix = p;
	}

	return matrix;	
}

const MatrixXd *SmoothObjCache::readEntry(int id1, int id2) const {
	sortIds(id1, id2);
	auto it = data.find(id1);
	assert(it != data.end());
	auto it2 = it->second.find(id2);
	assert(it2 != it->second.end());
	return it2->second;	
}

SmoothObjCache SmoothObjCache::defaultCache;

void SepSmoothObjNode::init(int phase, const DefaultStaticInput &nodeInput) {
	if(phase == 0) {	
		checkConstraintRows(*nodeInput.A, varDim);

		A = nodeInput.A;
		L = nodeInput.L;

		AAT = (*A) * A->transpose() / nodeInput.L;
	
		//Preallocate cache entries for pair matrices
		//this way we can update the cache without worrying
		//about synchronization
		for(int i = 0; i < numVars; ++i) {
			if (i != varId_) {
				cache.allocateEntry(varId_, i);
			}
		}
	}
	/*else if(phase == 1) {
		for(int i = 0; i < numVars; i++) {
			if(i != varId) { // Todo check netconfig
				MatrixXd *matrix = new MatrixXd();
				const MatrixXd &other = static_cast<const SmoothObjNode *>(scheduler->getNode(i))->AAT;
				pinv(AAT + other, *matrix);						
				cache.updateEntry(varId, i, matrix);
			}
		}	
	}*/
}

SmoothObjMasterInfo SepSmoothObjNode::getInfoAsMaster(
												   int slaveId,
												   const SepSmoothNodeInput& input) const {
	SmoothObjMasterInfo info;

	info.AAT = &AAT;	
	info.scaledGrad.reset(new VectorXd(varDim));
	grad_(*input, *info.scaledGrad);
	*info.scaledGrad /= L;
	info.Ad.reset(new VectorXd());
	*info.Ad = (*A) * (*info.scaledGrad);

	return info;
}

void SepSmoothObjNode::updateAsMaster(
								   const SmoothObjMasterInfo &myInfo,
								   int slaveId,
								   const SmoothObjSlaveInfo &slaveInfo,
								   Eigen::VectorXd& masterUpdate) const {
	masterUpdate = A->transpose() * *slaveInfo.lagrange / L - *myInfo.scaledGrad;
}

void SepSmoothObjNode::updateAsSlave(int masterId,
								  const SmoothObjMasterInfo &masterInfo,
								  const SepSmoothNodeInput &input,
								  SmoothObjSlaveInfo& slaveInfo,
								  Eigen::VectorXd& slaveUpdate) const {
	const MatrixXd *pairMatrix = cache.readEntry(this->varId_, masterId);
	
	if(pairMatrix == 0) { //first interaction with master
		pairMatrix = new MatrixXd();
		pinv(AAT + *masterInfo.AAT, const_cast<MatrixXd &>(*pairMatrix));		
		pairMatrix = cache.updateEntry(varId_, masterId, pairMatrix);
	}

	VectorXd grad;
	grad_(*input, grad);
	grad /= L;

	Platform::sleepCurrentThread(50);

	slaveInfo.lagrange.reset(new VectorXd());
	*slaveInfo.lagrange = *pairMatrix * (*masterInfo.Ad + (*A) * grad);
	slaveUpdate = A->transpose() * *slaveInfo.lagrange / L - grad;
}

void SepSmoothObjNode::pinv(const MatrixXd &m, MatrixXd &out) {
	JacobiSVD<MatrixXd> svd(m, ComputeThinU | ComputeThinV);
	MatrixXd V = svd.matrixV();
	MatrixXd U = svd.matrixU();
	VectorXd S = svd.singularValues();

	double epsilon = 1e-8 * S(0);
	int j;

	for(j = S.size() - 1; j >= 0 && S(j) <= epsilon; --j);
	int k = j+1;
	
	if(k == 0) {out = MatrixXd::Zero(m.rows(), m.cols());}
	else {
		for(j = 0; j < k; ++j) {V.col(j) /= S(j);}
	}

	out = U.leftCols(k) * V.leftCols(k).transpose();
}

void SepSmoothObjNode::checkConstraintRows(const Eigen::MatrixXd &A, int varDim) {
	int numConstraints = A.rows();
	int n = varDim;

	int numNZ = 0;
	for(int j = 0; j < numConstraints; j++) {
		int k = 0;
		for(; k < n && A(j,k) == 0; k++);
		if(k < n) {numNZ++;}
	}
	if(numNZ > n) {throw InvalidRowsException();}
}

