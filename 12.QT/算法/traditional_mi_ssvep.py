from __future__ import annotations

import json
import re
from dataclasses import asdict
from datetime import datetime
from fractions import Fraction
from pathlib import Path
from typing import Dict, List, Tuple

import mne
import numpy as np
import pandas as pd
from mne.decoding import CSP
from scipy.signal import resample_poly
from sklearn.isotonic import IsotonicRegression
from sklearn.linear_model import LogisticRegression
from sklearn.cross_decomposition import CCA
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix, f1_score, precision_score, recall_score
from sklearn.model_selection import StratifiedKFold
from sklearn.svm import SVC

from traditional_config import TraditionalFusionConfig


EPS = 1e-8

CSV_CN_MAP = {
    "subject_id": "被试ID",
    "trial": "试次",
    "fold": "折",
    "n_train": "训练样本数",
    "n_test": "测试样本数",
    "n_trials": "总试次数",
    "n_left": "左类样本数",
    "n_right": "右类样本数",
    "offset_sec": "偏移时间",
    "offset_source": "偏移来源",
    "source": "来源",
    "window_sec": "窗口时长",
    "threshold": "阈值",
    "accuracy": "准确率",
    "accuracy_mean": "准确率均值",
    "accuracy_std": "准确率标准差",
    "precision": "精确率",
    "precision_mean": "精确率均值",
    "recall": "召回率",
    "recall_mean": "召回率均值",
    "f1": "F1",
    "f1_mean": "F1均值",
    "f1_std": "F1标准差",
    "w_mi": "w_mi",
    "w_ssvep": "w_ssvep",
    "w_mi_mean": "w_mi均值",
    "w_ssvep_mean": "w_ssvep均值",
    "weight_source": "权重来源",
    "mi_acc_est": "MI估计准确率",
    "ssvep_acc_est": "SSVEP估计准确率",
    "mi_acc_est_mean": "MI估计准确率均值",
    "ssvep_acc_est_mean": "SSVEP估计准确率均值",
    "acc_fused": "融合准确率",
    "acc_mi": "MI准确率",
    "acc_ssvep": "SSVEP准确率",
    "acc_mi_mean": "MI准确率均值",
    "acc_ssvep_mean": "SSVEP准确率均值",
    "f1_fused": "融合F1",
    "acc_selected": "策略后准确率",
    "acc_riemann_block_diag": "块对角RMDM准确率",
    "acc_riemann_full": "联合RMDM准确率",
    "f1_selected": "策略后F1",
    "decision_mode": "决策模式",
    "acc_train_fused": "训练融合准确率",
    "acc_train_mi": "训练MI准确率",
    "acc_train_ssvep": "训练SSVEP准确率",
    "acc_train_riemann_block_diag": "训练块对角RMDM准确率",
    "acc_train_riemann_full": "训练联合RMDM准确率",
    "acc_riemann_block_diag_mean": "块对角RMDM准确率均值",
    "acc_riemann_full_mean": "联合RMDM准确率均值",
    "f1_ssvep": "SSVEP F1",
    "mean_decision_time": "平均决策时间",
    "median_decision_time": "中位决策时间",
    "p90_decision_time": "P90决策时间",
    "fallback_last_window_rate": "回退到最后窗口比例",
    "artifact_removed": "伪迹剔除数量",
    "artifact_keep_ratio": "伪迹保留比例",
    "y_true": "真实标签",
    "y_pred": "预测标签",
    "p_left": "左类概率",
    "p_right": "右类概率",
    "candidate_id": "候选ID",
    "run_root": "运行目录",
    "mi_bands": "MI频带",
    "ssvep_fb_bands": "SSVEP滤波器组频带",
    "ssvep_harmonics": "SSVEP谐波数",
    "score_scale": "评分缩放",
    "fusion_weight_mode": "融合权重模式",
    "fusion_weight_mi": "MI固定权重",
    "oof_splits": "OOF分层数",
    "confidence_gamma": "置信度gamma",
    "confidence_floor": "置信度下限",
}


def _to_csv_cn(df: pd.DataFrame, path: Path) -> None:
    df.rename(columns=CSV_CN_MAP).to_csv(path, index=False, encoding="utf-8-sig")


def _normalize_name(name: str) -> str:
    return re.sub(r"[^A-Z0-9]", "", name.upper())


def _bandpass_epochs(x: np.ndarray, sfreq: float, band: Tuple[float, float], order: int) -> np.ndarray:
    low, high = band
    nyq = sfreq * 0.5
    if low <= 0 or high >= nyq or low >= high:
        raise ValueError(f"Invalid band ({low}, {high}) for sfreq={sfreq}")
    # Use MNE FIR filtering for stability on this environment.
    return mne.filter.filter_data(
        data=x.astype(np.float64, copy=False),
        sfreq=sfreq,
        l_freq=low,
        h_freq=high,
        method="fir",
        verbose="ERROR",
    )


class MIFBCSPBranch:
    def __init__(self, cfg: TraditionalFusionConfig) -> None:
        self.cfg = cfg
        self.csp_list_: List[CSP] = []
        self.mean_: np.ndarray | None = None
        self.std_: np.ndarray | None = None
        self.clf = SVC(
            kernel="linear",
            probability=True,
            random_state=cfg.random_state,
        )

    def _log_ratio(self, tag: str, count: int, total: int) -> None:
        if count > 0 and total > 0:
            ratio = 100.0 * float(count) / float(total)
            print(f"[mi-fbcsp] {tag}: {count}/{total} ({ratio:.4f}%)", flush=True)

    def _features(self, x_bands: List[np.ndarray], fit: bool, y: np.ndarray | None = None) -> np.ndarray:
        chunks = []
        if fit:
            self.csp_list_ = []

        for b_idx, xb in enumerate(x_bands):
            xb64 = xb.astype(np.float64, copy=False)
            if fit:
                csp = CSP(
                    n_components=int(self.cfg.mi_csp_components),
                    reg=None,
                    log=False,
                    cov_est="epoch",
                )
                band_feat = csp.fit_transform(xb64, y)
                self.csp_list_.append(csp)
            else:
                csp = self.csp_list_[b_idx]
                band_feat = csp.transform(xb64)

            nonpos = int(np.sum(band_feat <= 0.0))
            self._log_ratio(
                f"{('fit' if fit else 'predict')} band={b_idx} raw_nonpositive",
                nonpos,
                int(band_feat.size),
            )

            # Safe log-power features: avoid -inf/inf from near-zero powers.
            band_feat = np.asarray(band_feat, dtype=np.float64)
            band_feat = np.log(np.maximum(band_feat, EPS))
            band_feat = np.nan_to_num(band_feat, nan=0.0, posinf=0.0, neginf=0.0)
            chunks.append(np.asarray(band_feat, dtype=np.float32))

        feat = np.concatenate(chunks, axis=1)
        return feat

    def fit(self, x_bands: List[np.ndarray], y: np.ndarray, sfreq: float) -> None:
        self.sfreq_ = float(sfreq)
        feat = self._features(x_bands, fit=True, y=y)
        self.mean_ = feat.mean(axis=0, keepdims=True)
        self.std_ = feat.std(axis=0, keepdims=True)
        self.std_ = np.where(self.std_ < EPS, 1.0, self.std_)
        feat = (feat - self.mean_) / self.std_
        nonfinite = int(np.sum(~np.isfinite(feat)))
        self._log_ratio("fit standardized_nonfinite", nonfinite, int(feat.size))
        feat = np.nan_to_num(feat, nan=0.0, posinf=0.0, neginf=0.0)
        self.clf.fit(feat, y)

    def predict_proba(self, x_bands: List[np.ndarray]) -> np.ndarray:
        feat = self._features(x_bands, fit=False)
        feat = (feat - self.mean_) / self.std_
        nonfinite = int(np.sum(~np.isfinite(feat)))
        self._log_ratio("predict standardized_nonfinite", nonfinite, int(feat.size))
        feat = np.nan_to_num(feat, nan=0.0, posinf=0.0, neginf=0.0)
        p = self.clf.predict_proba(feat)
        if p.shape[1] != 2:
            raise ValueError("MI branch expects binary probability outputs")
        classes = list(self.clf.classes_)
        if classes == [0, 1]:
            return p
        try:
            i0 = classes.index(0)
            i1 = classes.index(1)
            return p[:, [i0, i1]]
        except ValueError:
            raise ValueError("MI branch classes must contain labels 0 and 1")


class FBCCABranch:
    def __init__(self, cfg: TraditionalFusionConfig) -> None:
        self.cfg = cfg

    def fit(self, x_bands: List[np.ndarray], y: np.ndarray, sfreq: float) -> None:
        self.sfreq_ = float(sfreq)
        n_times = int(x_bands[0].shape[-1])
        t = np.arange(n_times, dtype=np.float64) / self.sfreq_

        refs = []
        for f in self.cfg.ssvep_freqs:
            basis = []
            for h in range(1, int(self.cfg.ssvep_harmonics) + 1):
                basis.append(np.sin(2.0 * np.pi * h * float(f) * t))
                basis.append(np.cos(2.0 * np.pi * h * float(f) * t))
            refs.append(np.asarray(basis, dtype=np.float64))
        self.reference_signals_ = np.asarray(refs, dtype=np.float64)

    def _cca_abs_corr(self, trial_ch_time: np.ndarray, ref_comp_time: np.ndarray) -> float:
        cca = CCA(n_components=1, max_iter=1000)
        try:
            cca.fit(trial_ch_time.T, ref_comp_time.T)
            x_c, y_c = cca.transform(trial_ch_time.T, ref_comp_time.T)
        except (ValueError, np.linalg.LinAlgError, FloatingPointError):
            return 0.0

        x0 = x_c[:, 0]
        y0 = y_c[:, 0]
        xc = x0 - np.mean(x0)
        yc = y0 - np.mean(y0)
        reg = float(self.cfg.cca_reg)
        denom = np.sqrt(np.sum(xc * xc) + reg) * np.sqrt(np.sum(yc * yc) + reg)
        if not np.isfinite(denom) or denom <= 0.0:
            return 0.0
        corr = float(np.dot(xc, yc) / denom)
        if not np.isfinite(corr):
            return 0.0
        return float(abs(corr))

    def _scores(self, x_bands: List[np.ndarray]) -> np.ndarray:
        n_trials = x_bands[0].shape[0]
        n_classes = self.reference_signals_.shape[0]
        scores = np.zeros((n_trials, n_classes), dtype=np.float64)

        for b_idx, xb in enumerate(x_bands):
            w = (b_idx + 1) ** (-self.cfg.ssvep_weight_a) + self.cfg.ssvep_weight_b
            for i in range(n_trials):
                trial = xb[i]
                for c in range(n_classes):
                    rho = self._cca_abs_corr(trial, self.reference_signals_[c])
                    scores[i, c] += w * (rho ** 2)
        return scores

    def predict_proba(self, x_bands: List[np.ndarray]) -> np.ndarray:
        scores = self._scores(x_bands)
        scores = scores * float(self.cfg.score_scale)
        scores = scores - scores.max(axis=1, keepdims=True)
        exp_scores = np.exp(scores)
        probs = exp_scores / (exp_scores.sum(axis=1, keepdims=True) + EPS)
        return probs.astype(np.float64)


class CovRMDMClassifier:
    def __init__(self, cfg: TraditionalFusionConfig, mode: str) -> None:
        if mode not in {"block_diag", "full"}:
            raise ValueError("CovRMDMClassifier mode must be 'block_diag' or 'full'")
        self.cfg = cfg
        self.mode = mode
        self.classes_ = np.asarray([0, 1], dtype=np.int64)
        self.class_means_: Dict[int, np.ndarray] = {}

    @staticmethod
    def _center(epoch: np.ndarray) -> np.ndarray:
        return epoch - epoch.mean(axis=1, keepdims=True)

    def _cov(self, x: np.ndarray) -> np.ndarray:
        n_t = max(1, int(x.shape[1]))
        c = (x @ x.T) / float(n_t)
        tr = float(np.trace(c))
        if tr <= EPS:
            tr = 1.0
        c = c / tr
        n_ch = c.shape[0]
        alpha = float(self.cfg.riemann_cov_shrinkage)
        if alpha > 0.0:
            c = (1.0 - alpha) * c + alpha * (np.trace(c) / max(n_ch, 1)) * np.eye(n_ch)
        c = 0.5 * (c + c.T)
        c = c + (EPS * np.eye(n_ch))
        return c

    def _trial_cov(self, x_mi: np.ndarray, x_ss: np.ndarray) -> np.ndarray:
        c_mi = self._cov(self._center(x_mi))
        c_ss = self._cov(self._center(x_ss))
        if self.mode == "block_diag":
            out = np.zeros((c_mi.shape[0] + c_ss.shape[0], c_mi.shape[1] + c_ss.shape[1]), dtype=np.float64)
            out[: c_mi.shape[0], : c_mi.shape[1]] = c_mi
            out[c_mi.shape[0] :, c_mi.shape[1] :] = c_ss
            return out

        x_all = np.concatenate([self._center(x_mi), self._center(x_ss)], axis=0)
        return self._cov(x_all)

    @staticmethod
    def _matrix_log(spd: np.ndarray) -> np.ndarray:
        vals, vecs = np.linalg.eigh(spd)
        vals = np.clip(vals, EPS, None)
        return (vecs * np.log(vals)) @ vecs.T

    def _log_euclid_mean(self, mats: List[np.ndarray]) -> np.ndarray:
        logs = [self._matrix_log(m) for m in mats]
        mean_log = np.mean(np.stack(logs, axis=0), axis=0)
        vals, vecs = np.linalg.eigh(mean_log)
        return (vecs * np.exp(vals)) @ vecs.T

    @staticmethod
    def _distance(spd_a: np.ndarray, spd_b: np.ndarray) -> float:
        la = CovRMDMClassifier._matrix_log(spd_a)
        lb = CovRMDMClassifier._matrix_log(spd_b)
        d = la - lb
        return float(np.sqrt(np.sum(d * d)))

    def fit(self, x_mi: np.ndarray, x_ss: np.ndarray, y: np.ndarray) -> None:
        covs = [self._trial_cov(x_mi[i], x_ss[i]) for i in range(y.shape[0])]
        self.class_means_.clear()
        for cls in self.classes_:
            cls_covs = [c for c, yy in zip(covs, y) if int(yy) == int(cls)]
            if not cls_covs:
                raise ValueError(f"No samples for class {cls} in CovRMDMClassifier")
            self.class_means_[int(cls)] = self._log_euclid_mean(cls_covs)

    def predict_proba(self, x_mi: np.ndarray, x_ss: np.ndarray) -> np.ndarray:
        out = np.zeros((x_mi.shape[0], 2), dtype=np.float64)
        for i in range(x_mi.shape[0]):
            c = self._trial_cov(x_mi[i], x_ss[i])
            d0 = self._distance(c, self.class_means_[0])
            d1 = self._distance(c, self.class_means_[1])
            s = np.asarray([-d0, -d1], dtype=np.float64)
            s = s - np.max(s)
            e = np.exp(s)
            out[i] = e / (np.sum(e) + EPS)
        return out


class LateFusionClassifier:
    def __init__(self, cfg: TraditionalFusionConfig) -> None:
        self.cfg = cfg
        self.mi = MIFBCSPBranch(cfg)
        self.ssvep = FBCCABranch(cfg)
        self.w_mi_ = 0.5
        self.w_ssvep_ = 0.5
        self.mi_acc_est_ = None
        self.ssvep_acc_est_ = None
        self.weight_source_ = "unset"
        self.calibration_source_ = "disabled"
        self.mi_calibrator_ = None
        self.ssvep_calibrator_ = None

    def _compute_auto_weight(self, mi_acc: float, ssvep_acc: float) -> float:
        denom = mi_acc + ssvep_acc + EPS
        w = float(mi_acc / denom)
        return float(np.clip(w, self.cfg.fusion_weight_min, self.cfg.fusion_weight_max))

    @staticmethod
    def _safe_logit(p: np.ndarray) -> np.ndarray:
        p_clip = np.clip(p, EPS, 1.0 - EPS)
        return np.log(p_clip / (1.0 - p_clip))

    def _fit_binary_calibrator(self, y: np.ndarray, p_pos: np.ndarray):
        if self.cfg.fusion_calibration_method == "isotonic":
            cal = IsotonicRegression(out_of_bounds="clip")
            cal.fit(np.asarray(p_pos, dtype=np.float64), y.astype(np.int32))
            return cal

        x = self._safe_logit(np.asarray(p_pos, dtype=np.float64)).reshape(-1, 1)
        cal = LogisticRegression(solver="lbfgs", random_state=self.cfg.random_state)
        cal.fit(x, y.astype(np.int32))
        return cal

    def _apply_binary_calibrator(self, p: np.ndarray, calibrator) -> np.ndarray:
        p_pos = np.asarray(p[:, 1], dtype=np.float64)
        if isinstance(calibrator, IsotonicRegression):
            p1 = calibrator.predict(p_pos)
        else:
            x = self._safe_logit(p_pos).reshape(-1, 1)
            p1 = calibrator.predict_proba(x)[:, 1]
        p1 = np.clip(p1, EPS, 1.0 - EPS)
        p0 = 1.0 - p1
        out = np.stack([p0, p1], axis=1)
        out = out / (out.sum(axis=1, keepdims=True) + EPS)
        return out

    def _fit_probability_calibrators(self, y: np.ndarray, p_mi: np.ndarray, p_ssvep: np.ndarray) -> None:
        labels, counts = np.unique(y, return_counts=True)
        if labels.size != 2 or np.min(counts) < 2:
            self.mi_calibrator_ = None
            self.ssvep_calibrator_ = None
            self.calibration_source_ = "disabled"
            return

        self.mi_calibrator_ = self._fit_binary_calibrator(y, p_mi[:, 1])
        self.ssvep_calibrator_ = self._fit_binary_calibrator(y, p_ssvep[:, 1])

    def _apply_probability_calibration(self, p_mi: np.ndarray, p_ssvep: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
        if not self.cfg.fusion_calibration_enabled:
            return p_mi, p_ssvep
        if self.mi_calibrator_ is None or self.ssvep_calibrator_ is None:
            return p_mi, p_ssvep
        return self._apply_binary_calibrator(p_mi, self.mi_calibrator_), self._apply_binary_calibrator(p_ssvep, self.ssvep_calibrator_)

    def _estimate_oof_modality_proba(
        self,
        x_mi_bands: List[np.ndarray],
        x_ssvep_bands: List[np.ndarray],
        y: np.ndarray,
        sfreq: float,
    ) -> Tuple[np.ndarray, np.ndarray] | None:
        n_samples = y.shape[0]
        labels, counts = np.unique(y, return_counts=True)
        if labels.size != 2:
            return None

        max_splits = int(np.min(counts))
        n_splits = min(self.cfg.fusion_oof_splits, max_splits)
        if n_splits < 2:
            return None

        splitter = StratifiedKFold(n_splits=n_splits, shuffle=True, random_state=self.cfg.random_state + 7)
        oof_mi = np.zeros((n_samples, 2), dtype=np.float64)
        oof_ss = np.zeros((n_samples, 2), dtype=np.float64)

        for tr_idx, va_idx in splitter.split(np.zeros((n_samples, 1), dtype=np.float32), y):
            mi_tmp = MIFBCSPBranch(self.cfg)
            ss_tmp = FBCCABranch(self.cfg)

            mi_tmp.fit([band[tr_idx] for band in x_mi_bands], y[tr_idx], sfreq)
            ss_tmp.fit([band[tr_idx] for band in x_ssvep_bands], y[tr_idx], sfreq)

            oof_mi[va_idx] = mi_tmp.predict_proba([band[va_idx] for band in x_mi_bands])
            oof_ss[va_idx] = ss_tmp.predict_proba([band[va_idx] for band in x_ssvep_bands])

        return oof_mi, oof_ss

    def fit(self, x_mi_bands: List[np.ndarray], x_ssvep_bands: List[np.ndarray], y: np.ndarray, sfreq: float) -> None:
        need_oof = self.cfg.fusion_oof_enabled and (self.cfg.fusion_weight_mode == "auto" or self.cfg.fusion_calibration_enabled)
        oof_proba = self._estimate_oof_modality_proba(x_mi_bands, x_ssvep_bands, y, sfreq) if need_oof else None

        self.mi.fit(x_mi_bands, y, sfreq)
        self.ssvep.fit(x_ssvep_bands, y, sfreq)

        p_mi_train = self.mi.predict_proba(x_mi_bands)
        p_ssvep_train = self.ssvep.predict_proba(x_ssvep_bands)

        if self.cfg.fusion_weight_mode == "fixed":
            w = float(self.cfg.fusion_weight_mi)
            self.weight_source_ = "fixed"
            self.mi_acc_est_ = None
            self.ssvep_acc_est_ = None
        else:
            if oof_proba is not None:
                p_mi_ref, p_ss_ref = oof_proba
                self.weight_source_ = "oof"
            else:
                p_mi_ref, p_ss_ref = p_mi_train, p_ssvep_train
                self.weight_source_ = "in_sample"

            mi_acc = float(accuracy_score(y, np.argmax(p_mi_ref, axis=1)))
            ss_acc = float(accuracy_score(y, np.argmax(p_ss_ref, axis=1)))
            self.mi_acc_est_ = mi_acc
            self.ssvep_acc_est_ = ss_acc
            w = self._compute_auto_weight(mi_acc, ss_acc)

        if self.cfg.fusion_calibration_enabled:
            if oof_proba is not None:
                p_mi_cal, p_ss_cal = oof_proba
                self.calibration_source_ = "oof"
            else:
                p_mi_cal, p_ss_cal = p_mi_train, p_ssvep_train
                self.calibration_source_ = "in_sample"
            self._fit_probability_calibrators(y, p_mi_cal, p_ss_cal)
        else:
            self.mi_calibrator_ = None
            self.ssvep_calibrator_ = None
            self.calibration_source_ = "disabled"

        w = float(np.clip(w, self.cfg.fusion_weight_min, self.cfg.fusion_weight_max))
        self.w_mi_ = w
        self.w_ssvep_ = 1.0 - w

    def _fuse_branch_proba(self, p_mi: np.ndarray, p_ssvep: np.ndarray) -> np.ndarray:
        if not self.cfg.fusion_confidence_enabled:
            p_fused = self.w_mi_ * p_mi + self.w_ssvep_ * p_ssvep
            w_mi_dyn = np.full(p_mi.shape[0], self.w_mi_, dtype=np.float64)
            w_ss_dyn = np.full(p_mi.shape[0], self.w_ssvep_, dtype=np.float64)
        else:
            conf_floor = float(self.cfg.fusion_confidence_floor)
            gamma = float(self.cfg.fusion_confidence_gamma)

            conf_mi = np.clip(np.abs(p_mi[:, 1] - p_mi[:, 0]), conf_floor, 1.0)
            conf_ss = np.clip(np.abs(p_ssvep[:, 1] - p_ssvep[:, 0]), conf_floor, 1.0)

            score_mi = self.w_mi_ * np.power(conf_mi, gamma)
            score_ss = self.w_ssvep_ * np.power(conf_ss, gamma)
            norm = score_mi + score_ss + EPS

            w_mi_dyn = score_mi / norm
            w_ss_dyn = score_ss / norm
            p_fused = w_mi_dyn[:, None] * p_mi + w_ss_dyn[:, None] * p_ssvep

        policy = self.cfg.fusion_disagree_policy
        if policy == "none":
            return p_fused

        pred_mi = np.argmax(p_mi, axis=1)
        pred_ss = np.argmax(p_ssvep, axis=1)
        disagree = pred_mi != pred_ss
        if not np.any(disagree):
            return p_fused

        out = p_fused.copy()
        if policy == "prefer_ssvep":
            out[disagree] = p_ssvep[disagree]
        elif policy == "prefer_mi":
            out[disagree] = p_mi[disagree]
        elif policy == "prefer_higher_confidence":
            conf_mi = np.abs(p_mi[:, 1] - p_mi[:, 0])
            conf_ss = np.abs(p_ssvep[:, 1] - p_ssvep[:, 0])
            use_mi = disagree & (conf_mi >= conf_ss)
            use_ss = disagree & (~use_mi)
            out[use_mi] = p_mi[use_mi]
            out[use_ss] = p_ssvep[use_ss]
        elif policy == "prefer_higher_weight":
            use_mi = disagree & (w_mi_dyn >= w_ss_dyn)
            use_ss = disagree & (~use_mi)
            out[use_mi] = p_mi[use_mi]
            out[use_ss] = p_ssvep[use_ss]
        return out

    def predict_with_branches(
        self,
        x_mi_bands: List[np.ndarray],
        x_ssvep_bands: List[np.ndarray],
    ) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
        p_mi = self.mi.predict_proba(x_mi_bands)
        p_ssvep = self.ssvep.predict_proba(x_ssvep_bands)
        p_mi, p_ssvep = self._apply_probability_calibration(p_mi, p_ssvep)
        p_fused = self._fuse_branch_proba(p_mi, p_ssvep)
        return p_fused, p_mi, p_ssvep

    def predict_proba(self, x_mi_bands: List[np.ndarray], x_ssvep_bands: List[np.ndarray]) -> np.ndarray:
        p_fused, _, _ = self.predict_with_branches(x_mi_bands, x_ssvep_bands)
        return p_fused


class SubjectDataset:
    def __init__(
        self,
        subject_id: str,
        x_mi: np.ndarray,
        x_ssvep: np.ndarray,
        y: np.ndarray,
        trials: np.ndarray,
        offset_sec: float,
        offset_source: str,
    ) -> None:
        self.subject_id = subject_id
        self.x_mi = x_mi
        self.x_ssvep = x_ssvep
        self.y = y
        self.trials = trials
        self.offset_sec = float(offset_sec)
        self.offset_source = str(offset_source)


def _apply_artifact_rejection(
    x_mi: np.ndarray,
    x_ssvep: np.ndarray,
    y: np.ndarray,
    trials: np.ndarray,
    cfg: TraditionalFusionConfig,
) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, Dict[str, float]]:
    n_total = int(y.shape[0])
    stats = {
        "artifact_total": float(n_total),
        "artifact_removed": 0.0,
        "artifact_kept": float(n_total),
        "artifact_keep_ratio": 1.0,
    }

    if not cfg.artifact_reject_enabled:
        return x_mi, x_ssvep, y, trials, stats

    peak_uv = np.maximum(
        np.max(np.abs(x_mi), axis=(1, 2)),
        np.max(np.abs(x_ssvep), axis=(1, 2)),
    ) * 1e6
    keep_peak = peak_uv <= float(cfg.artifact_peak_uv)

    mi_lv = np.log(np.var(x_mi, axis=2, ddof=0) + EPS).mean(axis=1)
    ss_lv = np.log(np.var(x_ssvep, axis=2, ddof=0) + EPS).mean(axis=1)
    lv = 0.5 * (mi_lv + ss_lv)
    med = float(np.median(lv))
    mad = float(np.median(np.abs(lv - med)))
    if mad < EPS:
        keep_z = np.ones_like(keep_peak, dtype=bool)
    else:
        robust_z = 0.6745 * (lv - med) / (mad + EPS)
        keep_z = np.abs(robust_z) <= float(cfg.artifact_logvar_z)

    keep = keep_peak & keep_z

    labels = np.unique(y)
    per_class_ok = all(int(np.sum(keep & (y == lb))) >= int(cfg.artifact_min_class_epochs) for lb in labels)
    if (not per_class_ok) or (int(np.sum(keep)) < 2) or (np.unique(y[keep]).size < 2):
        # Fallback to original data to avoid collapsing a class in small-sample runs.
        return x_mi, x_ssvep, y, trials, stats

    kept = int(np.sum(keep))
    removed = n_total - kept
    stats = {
        "artifact_total": float(n_total),
        "artifact_removed": float(removed),
        "artifact_kept": float(kept),
        "artifact_keep_ratio": float(kept / max(n_total, 1)),
    }

    return x_mi[keep], x_ssvep[keep], y[keep], trials[keep], stats


def _read_events(csv_path: Path, cfg: TraditionalFusionConfig) -> pd.DataFrame:
    df = pd.read_csv(csv_path, comment="=")
    needed = {cfg.trial_col, cfg.direction_col, cfg.time_col, "event_type"}
    miss = needed - set(df.columns)
    if miss:
        raise ValueError(f"{csv_path.name} missing columns: {sorted(miss)}")

    event_df = df[df["event_type"] == cfg.event_type].copy()
    if cfg.condition_col in event_df.columns and cfg.condition_filter:
        event_df = event_df[event_df[cfg.condition_col] == cfg.condition_filter]

    event_df[cfg.direction_col] = event_df[cfg.direction_col].astype(str).str.lower()
    event_df = event_df[event_df[cfg.direction_col].isin(["left", "right"])]

    event_df = event_df[[cfg.trial_col, cfg.direction_col, cfg.time_col]].copy()
    event_df = event_df.dropna().sort_values(cfg.trial_col)

    event_df["label"] = (event_df[cfg.direction_col] == "right").astype(np.int64)
    event_df["start_sec"] = event_df[cfg.time_col].astype(float)
    return event_df


def _channel_indices(raw_names: List[str], target_names: List[str], branch_name: str) -> List[int]:
    norm_map = {_normalize_name(ch): i for i, ch in enumerate(raw_names)}
    indices = []
    missing = []
    for name in target_names:
        key = _normalize_name(name)
        if key in norm_map:
            indices.append(norm_map[key])
        else:
            missing.append(name)

    if missing:
        raise ValueError(
            f"{branch_name} channels missing in BDF: {missing}. "
            f"Available: {raw_names}"
        )
    return indices


def _resample_epoch(epoch: np.ndarray, src_sfreq: float, dst_sfreq: float, expected_n: int) -> np.ndarray:
    if abs(src_sfreq - dst_sfreq) < 1e-9:
        out = epoch
    else:
        frac = Fraction(dst_sfreq / src_sfreq).limit_denominator(1000)
        out = resample_poly(epoch, frac.numerator, frac.denominator, axis=-1)

    if out.shape[-1] > expected_n:
        out = out[:, :expected_n]
    elif out.shape[-1] < expected_n:
        pad = np.zeros((out.shape[0], expected_n - out.shape[-1]), dtype=out.dtype)
        out = np.concatenate([out, pad], axis=-1)
    return out


def _build_offset_candidates(cfg: TraditionalFusionConfig) -> np.ndarray:
    step = float(cfg.auto_offset_step_sec)
    start = float(cfg.auto_offset_min_sec)
    stop = float(cfg.auto_offset_max_sec)
    return np.arange(start, stop + 0.5 * step, step, dtype=np.float64)


def _estimate_best_offset(
    raw,
    events: pd.DataFrame,
    ss_idx: List[int],
    cfg: TraditionalFusionConfig,
) -> Tuple[float, List[Dict[str, float]]]:
    raw_sfreq = float(raw.info["sfreq"])
    raw_window = int(round(cfg.imagery_window_sec * raw_sfreq))
    tgt_window = int(round(cfg.imagery_window_sec * cfg.target_sfreq))

    scan_rows: List[Dict[str, float]] = []
    best_offset = 0.0
    best_score = -1.0

    for offset in _build_offset_candidates(cfg):
        epochs = []
        labels = []

        for _, row in events.iterrows():
            start_sec = float(row["start_sec"]) + float(offset)
            start = int(round(start_sec * raw_sfreq))
            stop = start + raw_window
            if start < 0 or stop > raw.n_times:
                continue

            ss_epoch = raw.get_data(picks=ss_idx, start=start, stop=stop)
            ss_epoch = _resample_epoch(ss_epoch, raw_sfreq, cfg.target_sfreq, tgt_window)
            epochs.append(ss_epoch.astype(np.float32))
            labels.append(int(row["label"]))

        n_epochs = len(epochs)
        score = np.nan
        if n_epochs >= cfg.auto_offset_min_epochs:
            x = np.asarray(epochs, dtype=np.float32)
            y = np.asarray(labels, dtype=np.int64)
            if np.unique(y).size == 2:
                x_bands = [
                    _bandpass_epochs(x, cfg.target_sfreq, band, cfg.bandpass_order).astype(np.float32)
                    for band in cfg.ssvep_fb_bands
                ]
                ss = FBCCABranch(cfg)
                ss.fit(x_bands, y, sfreq=cfg.target_sfreq)
                p = ss.predict_proba(x_bands)
                score = float(accuracy_score(y, np.argmax(p, axis=1)))

        scan_rows.append(
            {
                "offset_sec": float(offset),
                "score": float(score) if not np.isnan(score) else np.nan,
                "n_epochs": float(n_epochs),
            }
        )

        if not np.isnan(score):
            if (score > best_score + 1e-12) or (abs(score - best_score) <= 1e-12 and abs(offset) < abs(best_offset)):
                best_score = score
                best_offset = float(offset)

    return best_offset, scan_rows


def _resolve_subject_offset(
    sid: str,
    bdf_path: Path,
    csv_path: Path,
    cfg: TraditionalFusionConfig,
) -> Tuple[float, str, List[Dict[str, float]]]:
    if sid in cfg.per_recording_offsets:
        return float(cfg.per_recording_offsets[sid]), "manual", []

    if not cfg.auto_offset_enabled:
        return 0.0, "default", []

    raw = mne.io.read_raw_bdf(str(bdf_path), preload=False, verbose="ERROR")
    events = _read_events(csv_path, cfg)
    ss_idx = _channel_indices(raw.ch_names, cfg.ssvep_channels, "SSVEP")

    best_offset, scan_rows = _estimate_best_offset(raw, events, ss_idx, cfg)
    return float(best_offset), "auto", scan_rows


def _load_subject_data(
    bdf_path: Path,
    csv_path: Path,
    cfg: TraditionalFusionConfig,
    offset_sec: float,
    offset_source: str,
) -> SubjectDataset:
    raw = mne.io.read_raw_bdf(str(bdf_path), preload=False, verbose="ERROR")
    raw_sfreq = float(raw.info["sfreq"])
    print(f"设备采样率: {raw_sfreq}")
    events = _read_events(csv_path, cfg)

    mi_idx = _channel_indices(raw.ch_names, cfg.mi_channels, "MI")
    ss_idx = _channel_indices(raw.ch_names, cfg.ssvep_channels, "SSVEP")

    raw_window = int(round(cfg.imagery_window_sec * raw_sfreq))
    tgt_window = int(round(cfg.imagery_window_sec * cfg.target_sfreq))

    mi_epochs = []
    ss_epochs = []
    labels = []
    trials = []

    for _, row in events.iterrows():
        start_sec = float(row["start_sec"]) + float(offset_sec)
        start = int(round(start_sec * raw_sfreq))
        stop = start + raw_window
        if start < 0 or stop > raw.n_times:
            continue

        mi_epoch = raw.get_data(picks=mi_idx, start=start, stop=stop)
        ss_epoch = raw.get_data(picks=ss_idx, start=start, stop=stop)

        mi_epoch = _resample_epoch(mi_epoch, raw_sfreq, cfg.target_sfreq, tgt_window)
        ss_epoch = _resample_epoch(ss_epoch, raw_sfreq, cfg.target_sfreq, tgt_window)

        mi_epochs.append(mi_epoch.astype(np.float32))
        ss_epochs.append(ss_epoch.astype(np.float32))
        labels.append(int(row["label"]))
        trials.append(int(row[cfg.trial_col]))

    if not mi_epochs:
        raise ValueError(f"No valid epochs extracted for {bdf_path.name}")

    x_mi = np.asarray(mi_epochs, dtype=np.float32)
    x_ss = np.asarray(ss_epochs, dtype=np.float32)
    y = np.asarray(labels, dtype=np.int64)
    tr = np.asarray(trials, dtype=np.int64)

    x_mi, x_ss, y, tr, artifact_stats = _apply_artifact_rejection(x_mi, x_ss, y, tr, cfg)

    ds = SubjectDataset(
        subject_id=bdf_path.stem,
        x_mi=x_mi,
        x_ssvep=x_ss,
        y=y,
        trials=tr,
        offset_sec=float(offset_sec),
        offset_source=offset_source,
    )
    ds.artifact_stats = artifact_stats
    return ds


def _discover_subject_pairs(data_dir: Path) -> List[Tuple[str, Path, Path]]:
    pairs = []
    for bdf in sorted(data_dir.glob("*.bdf")):
        csv = data_dir / f"{bdf.stem}.csv"
        if csv.exists():
            pairs.append((bdf.stem, bdf, csv))
    return pairs


def _subject_metrics(y_true: np.ndarray, y_pred: np.ndarray) -> Dict[str, float]:
    return {
        "accuracy": float(accuracy_score(y_true, y_pred)),
        "precision": float(precision_score(y_true, y_pred, zero_division=0)),
        "recall": float(recall_score(y_true, y_pred, zero_division=0)),
        "f1": float(f1_score(y_true, y_pred, zero_division=0)),
    }


def _ensure_dir(path: Path) -> Path:
    path.mkdir(parents=True, exist_ok=True)
    return path


def _timestamp() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def run_pipeline(cfg: TraditionalFusionConfig) -> Dict[str, object]:
    cfg.validate()

    work_dir = Path(__file__).resolve().parent
    data_dir = work_dir / cfg.data_dir
    if not data_dir.exists():
        raise FileNotFoundError(f"Data directory not found: {data_dir}")

    pairs = _discover_subject_pairs(data_dir)
    if not pairs:
        raise FileNotFoundError(f"No .bdf/.csv subject pairs found in {data_dir}")

    selected = set(cfg.subject_ids) if cfg.subject_ids else None
    if selected is not None:
        pairs = [x for x in pairs if x[0] in selected]
        if not pairs:
            raise ValueError(f"No matched subject IDs in data dir. Requested={cfg.subject_ids}")

    run_root = _ensure_dir(work_dir / cfg.results_dir / f"{_timestamp()}_MI_SSVEP_TRAD")

    with open(run_root / "config_snapshot.json", "w", encoding="utf-8") as f:
        json.dump(asdict(cfg), f, ensure_ascii=False, indent=2)

    subject_rows: List[Dict[str, object]] = []
    all_pred_rows: List[Dict[str, object]] = []
    resolved_offsets: Dict[str, float] = {}
    offset_rows: List[Dict[str, object]] = []

    for sid, bdf_path, csv_path in pairs:
        subj_dir = _ensure_dir(run_root / f"subject_{sid}")

        offset_sec, offset_source, offset_scan_rows = _resolve_subject_offset(sid, bdf_path, csv_path, cfg)
        resolved_offsets[sid] = float(offset_sec)
        print(
            f"[subject] {sid}: loading epochs | offset={offset_sec:+.3f}s ({offset_source})",
            flush=True,
        )
        offset_rows.append(
            {
                "subject_id": sid,
                "offset_sec": float(offset_sec),
                "offset_source": offset_source,
            }
        )
        if offset_scan_rows:
            _to_csv_cn(pd.DataFrame(offset_scan_rows), subj_dir / "offset_scan.csv")

        ds = _load_subject_data(
            bdf_path,
            csv_path,
            cfg,
            offset_sec=offset_sec,
            offset_source=offset_source,
        )
        artifact_stats = getattr(ds, "artifact_stats", None)
        if artifact_stats and artifact_stats.get("artifact_removed", 0.0) > 0:
            print(
                f"[subject] {sid}: artifact reject removed={int(artifact_stats['artifact_removed'])}/{int(artifact_stats['artifact_total'])}",
                flush=True,
            )
        if len(np.unique(ds.y)) != 2:
            raise ValueError(f"Subject {sid} is not binary after loading")

        # Precompute filter-bank epochs once per subject to avoid repeated filtering in CV.
        print(f"[subject] {sid}: precomputing MI filter bank", flush=True)
        mi_band_data = [
            _bandpass_epochs(ds.x_mi, cfg.target_sfreq, band, cfg.bandpass_order).astype(np.float32)
            for band in cfg.mi_bands
        ]
        print(f"[subject] {sid}: precomputing SSVEP filter bank", flush=True)
        ssvep_band_data = [
            _bandpass_epochs(ds.x_ssvep, cfg.target_sfreq, band, cfg.bandpass_order).astype(np.float32)
            for band in cfg.ssvep_fb_bands
        ]

        skf = StratifiedKFold(n_splits=cfg.n_splits, shuffle=True, random_state=cfg.random_state)

        fold_rows = []
        oof_pred = np.zeros((ds.y.shape[0], 2), dtype=np.float64)
        oof_label = ds.y.copy()

        for fold_idx, (train_idx, test_idx) in enumerate(skf.split(ds.x_mi, ds.y), start=1):
            print(f"[subject] {sid}: fold {fold_idx}/{cfg.n_splits}", flush=True)
            clf = LateFusionClassifier(cfg)

            x_mi_tr = [band[train_idx] for band in mi_band_data]
            x_mi_te = [band[test_idx] for band in mi_band_data]
            x_ss_tr = [band[train_idx] for band in ssvep_band_data]
            x_ss_te = [band[test_idx] for band in ssvep_band_data]
            y_tr, y_te = ds.y[train_idx], ds.y[test_idx]

            clf.fit(x_mi_tr, x_ss_tr, y_tr, sfreq=cfg.target_sfreq)
            p_fused, p_mi, p_ss = clf.predict_with_branches(x_mi_te, x_ss_te)
            p_fused_tr, p_mi_tr, p_ss_tr = clf.predict_with_branches(x_mi_tr, x_ss_tr)

            p_rbd = None
            p_rfull = None
            p_rbd_tr = None
            p_rfull_tr = None
            acc_train_rbd = np.nan
            acc_train_rfull = np.nan
            if cfg.riemann_block_diag_enabled:
                rbd = CovRMDMClassifier(cfg, mode="block_diag")
                rbd.fit(ds.x_mi[train_idx], ds.x_ssvep[train_idx], y_tr)
                p_rbd = rbd.predict_proba(ds.x_mi[test_idx], ds.x_ssvep[test_idx])
                p_rbd_tr = rbd.predict_proba(ds.x_mi[train_idx], ds.x_ssvep[train_idx])
                acc_train_rbd = float(accuracy_score(y_tr, np.argmax(p_rbd_tr, axis=1)))
            if cfg.riemann_full_enabled:
                rfull = CovRMDMClassifier(cfg, mode="full")
                rfull.fit(ds.x_mi[train_idx], ds.x_ssvep[train_idx], y_tr)
                p_rfull = rfull.predict_proba(ds.x_mi[test_idx], ds.x_ssvep[test_idx])
                p_rfull_tr = rfull.predict_proba(ds.x_mi[train_idx], ds.x_ssvep[train_idx])
                acc_train_rfull = float(accuracy_score(y_tr, np.argmax(p_rfull_tr, axis=1)))

            acc_train_fused = float(accuracy_score(y_tr, np.argmax(p_fused_tr, axis=1)))
            acc_train_mi = float(accuracy_score(y_tr, np.argmax(p_mi_tr, axis=1)))
            acc_train_ssvep = float(accuracy_score(y_tr, np.argmax(p_ss_tr, axis=1)))

            if cfg.subject_fusion_policy == "best_train":
                mode_scores = [
                    ("fused", acc_train_fused),
                    ("ssvep", acc_train_ssvep),
                    ("mi", acc_train_mi),
                ]
                if cfg.riemann_block_diag_enabled and np.isfinite(acc_train_rbd):
                    mode_scores.append(("riemann_block_diag", float(acc_train_rbd)))
                if cfg.riemann_full_enabled and np.isfinite(acc_train_rfull):
                    mode_scores.append(("riemann_full", float(acc_train_rfull)))
                selected_mode = max(mode_scores, key=lambda x: x[1])[0]
            else:
                selected_mode = "fused"

            if selected_mode == "ssvep":
                p_selected = p_ss
            elif selected_mode == "mi":
                p_selected = p_mi
            elif selected_mode == "riemann_block_diag" and p_rbd is not None:
                p_selected = p_rbd
            elif selected_mode == "riemann_full" and p_rfull is not None:
                p_selected = p_rfull
            else:
                p_selected = p_fused

            y_pred = np.argmax(p_selected, axis=1)
            y_pred_fused = np.argmax(p_fused, axis=1)
            y_mi = np.argmax(p_mi, axis=1)
            y_ss = np.argmax(p_ss, axis=1)
            y_rbd = np.argmax(p_rbd, axis=1) if p_rbd is not None else None
            y_rfull = np.argmax(p_rfull, axis=1) if p_rfull is not None else None

            oof_pred[test_idx] = p_selected

            fold_rows.append(
                {
                    "fold": fold_idx,
                    "n_train": int(len(train_idx)),
                    "n_test": int(len(test_idx)),
                    "decision_mode": selected_mode,
                    "w_mi": float(clf.w_mi_),
                    "w_ssvep": float(clf.w_ssvep_),
                    "weight_source": clf.weight_source_,
                    "mi_acc_est": clf.mi_acc_est_,
                    "ssvep_acc_est": clf.ssvep_acc_est_,
                    "acc_selected": float(accuracy_score(y_te, y_pred)),
                    "acc_fused": float(accuracy_score(y_te, y_pred_fused)),
                    "acc_mi": float(accuracy_score(y_te, y_mi)),
                    "acc_ssvep": float(accuracy_score(y_te, y_ss)),
                    "acc_riemann_block_diag": float(accuracy_score(y_te, y_rbd)) if y_rbd is not None else np.nan,
                    "acc_riemann_full": float(accuracy_score(y_te, y_rfull)) if y_rfull is not None else np.nan,
                    "acc_train_fused": acc_train_fused,
                    "acc_train_mi": acc_train_mi,
                    "acc_train_ssvep": acc_train_ssvep,
                    "acc_train_riemann_block_diag": float(acc_train_rbd) if np.isfinite(acc_train_rbd) else np.nan,
                    "acc_train_riemann_full": float(acc_train_rfull) if np.isfinite(acc_train_rfull) else np.nan,
                    "f1_selected": float(f1_score(y_te, y_pred, zero_division=0)),
                    "f1_fused": float(f1_score(y_te, y_pred_fused, zero_division=0)),
                }
            )

        y_hat = np.argmax(oof_pred, axis=1)
        metrics = _subject_metrics(oof_label, y_hat)
        cm = confusion_matrix(oof_label, y_hat, labels=[0, 1])
        report = classification_report(oof_label, y_hat, labels=[0, 1], digits=4, zero_division=0)

        fold_df = pd.DataFrame(fold_rows)
        _to_csv_cn(fold_df, subj_dir / "fold_metrics.csv")

        with open(subj_dir / "classification_report.txt", "w", encoding="utf-8") as f:
            f.write(report)

        np.save(subj_dir / "confusion_matrix.npy", cm)

        summary_row = {
            "subject_id": sid,
            "n_trials": int(ds.y.shape[0]),
            "n_left": int((ds.y == 0).sum()),
            "n_right": int((ds.y == 1).sum()),
            "offset_sec": float(ds.offset_sec),
            "offset_source": ds.offset_source,
            **metrics,
            "w_mi_mean": float(fold_df["w_mi"].mean()),
            "w_ssvep_mean": float(fold_df["w_ssvep"].mean()),
            "decision_mode": str(fold_df["decision_mode"].mode().iloc[0]) if "decision_mode" in fold_df.columns else "fused",
            "weight_source": str(fold_df["weight_source"].mode().iloc[0]),
            "mi_acc_est_mean": float(fold_df["mi_acc_est"].dropna().mean()) if fold_df["mi_acc_est"].notna().any() else np.nan,
            "ssvep_acc_est_mean": float(fold_df["ssvep_acc_est"].dropna().mean()) if fold_df["ssvep_acc_est"].notna().any() else np.nan,
            "acc_mi_mean": float(fold_df["acc_mi"].mean()),
            "acc_ssvep_mean": float(fold_df["acc_ssvep"].mean()),
            "acc_riemann_block_diag_mean": float(fold_df["acc_riemann_block_diag"].dropna().mean()) if fold_df["acc_riemann_block_diag"].notna().any() else np.nan,
            "acc_riemann_full_mean": float(fold_df["acc_riemann_full"].dropna().mean()) if fold_df["acc_riemann_full"].notna().any() else np.nan,
            "artifact_removed": float(artifact_stats["artifact_removed"]) if artifact_stats else 0.0,
            "artifact_keep_ratio": float(artifact_stats["artifact_keep_ratio"]) if artifact_stats else 1.0,
        }
        subject_rows.append(summary_row)

        if cfg.save_predictions:
            pred_df = pd.DataFrame(
                {
                    "trial": ds.trials,
                    "y_true": oof_label,
                    "y_pred": y_hat,
                    "p_left": oof_pred[:, 0],
                    "p_right": oof_pred[:, 1],
                }
            )
            _to_csv_cn(pred_df, subj_dir / "oof_predictions.csv")
            pred_df.insert(0, "subject_id", sid)
            all_pred_rows.extend(pred_df.to_dict(orient="records"))

        print(
            f"[subject] {sid}: done | acc={metrics['accuracy']:.4f}, f1={metrics['f1']:.4f}",
            flush=True,
        )

    subject_df = pd.DataFrame(subject_rows)
    _to_csv_cn(subject_df, run_root / "subject_summary.csv")

    overall = {
        "n_subjects": int(subject_df.shape[0]),
        "accuracy_mean": float(subject_df["accuracy"].mean()),
        "accuracy_std": float(subject_df["accuracy"].std(ddof=1)) if subject_df.shape[0] > 1 else 0.0,
        "f1_mean": float(subject_df["f1"].mean()),
        "f1_std": float(subject_df["f1"].std(ddof=1)) if subject_df.shape[0] > 1 else 0.0,
        "acc_mi_mean": float(subject_df["acc_mi_mean"].mean()),
        "acc_ssvep_mean": float(subject_df["acc_ssvep_mean"].mean()),
        "acc_riemann_block_diag_mean": float(subject_df["acc_riemann_block_diag_mean"].dropna().mean()) if subject_df["acc_riemann_block_diag_mean"].notna().any() else np.nan,
        "acc_riemann_full_mean": float(subject_df["acc_riemann_full_mean"].dropna().mean()) if subject_df["acc_riemann_full_mean"].notna().any() else np.nan,
    }

    with open(run_root / "overall_summary.json", "w", encoding="utf-8") as f:
        json.dump(overall, f, ensure_ascii=False, indent=2)

    if cfg.save_predictions and all_pred_rows:
        _to_csv_cn(pd.DataFrame(all_pred_rows), run_root / "all_oof_predictions.csv")

    with open(run_root / "resolved_offsets.json", "w", encoding="utf-8") as f:
        json.dump(resolved_offsets, f, ensure_ascii=False, indent=2)

    if offset_rows:
        _to_csv_cn(pd.DataFrame(offset_rows), run_root / "resolved_offsets.csv")

    return {
        "run_root": str(run_root),
        "overall": overall,
        "subject_summary": subject_rows,
    }
