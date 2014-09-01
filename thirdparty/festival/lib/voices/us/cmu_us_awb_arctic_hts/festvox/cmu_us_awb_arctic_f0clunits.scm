;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;                                                                     ;;;
;;;                     Carnegie Mellon University                      ;;;
;;;                         Copyright (c) 2003                          ;;;
;;;                        All Rights Reserved.                         ;;;
;;;                                                                     ;;;
;;; Permission is hereby granted, free of charge, to use and distribute ;;;
;;; this software and its documentation without restriction, including  ;;;
;;; without limitation the rights to use, copy, modify, merge, publish, ;;;
;;; distribute, sublicense, and/or sell copies of this work, and to     ;;;
;;; permit persons to whom this work is furnished to do so, subject to  ;;;
;;; the following conditions:                                           ;;;
;;;  1. The code must retain the above copyright notice, this list of   ;;;
;;;     conditions and the following disclaimer.                        ;;;
;;;  2. Any modifications must be clearly marked as such.               ;;;
;;;  3. Original authors' names are not deleted.                        ;;;
;;;  4. The authors' names are not used to endorse or promote products  ;;;
;;;     derived from this software without specific prior written       ;;;
;;;     permission.                                                     ;;;
;;;                                                                     ;;;
;;; CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK        ;;;
;;; DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING     ;;;
;;; ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT  ;;;
;;; SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE     ;;;
;;; FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES   ;;;
;;; WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN  ;;;
;;; AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,         ;;;
;;; ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF      ;;;
;;; THIS SOFTWARE.                                                      ;;;
;;;                                                                     ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;                                                                      ;;
;;;  An F0 model by unit selection                                        ;;
;;;  Continuing from Antoine Raux's (antoine@cs.cmu.edu) 11752 Project    ;;
;;;                                                                       ;;
;;;  Vaguely based on "Automatic Prosody Generation using                 ;;
;;;  suprasegmental unit selection", Malfrere, F. and Dutoit, T. and      ;;
;;;  Mertens, P., Proc. ESCA Workshop on Speech Synthesis, pp 323-327,    ;;
;;;  Australia, 1998                                                      ;;
;;;                                                                       ;;
;;;  Note this needs a post-1.4.3 version of ch_track                     ;;
;;;                                                                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Ensure this version of festival has been compiled with clunits module
(require_module 'clunits)
(require 'clunits) ;; runtime scheme support

(if (assoc 'cmu_us_awb_arctic_clunits voice-locations)
    (defvar cmu_us_awb_arctic::f0_clunits_dir 
      (cdr (assoc 'cmu_us_awb_arctic_clunits voice-locations)))
    (defvar cmu_us_awb_arctic::f0_clunits_dir (string-append (pwd) "/")))


(set! cmu_us_awb_arctic::f0_dt_params
      (list
       (list 'db_dir cmu_us_awb_arctic::f0_clunits_dir)
       '(name cmu_us_awb_arctic_f0)
       '(index_name cmu_us_awb_arctic_f0)
       '(f0_join_weight 10000.0)
       '(join_weights
         (10.0 0.5))
       '(trees_dir "festival/trees/")
       '(catalogue_dir "festival/clunits/")
       '(coeffs_dir "f0feat/")
       '(coeffs_ext ".f0feat")
       '(clunit_name_feat lisp_cmu_us_awb_arctic::f0_clunit_name)
       ;;  Run time parameters 
       '(join_method windowed)
       ;; if pitch mark extraction is bad this is better than the above
;       '(join_method smoothedjoin)
;       '(join_method modified_lpc)
       '(continuity_weight 5)
;       '(log_scores 1)  ;; good for high variance joins (not so good for ldom)
       '(optimal_coupling 1)
       '(extend_selections 2)
       '(pm_coeffs_dir "f0feat/")
       '(pm_coeffs_ext ".f0feat")
       '(sig_dir "wav/")
       '(sig_ext ".wav")
;       '(pm_coeffs_dir "lpc/")
;       '(pm_coeffs_ext ".lpc")
;       '(sig_dir "lpc/")
;       '(sig_ext ".res")
;       '(clunits_debug 1)
))

(defvar cmu_us_awb_arctic::f0_clunits_loaded nil)

(define (cmu_us_awb_arctic::f0_clunit_name i)
  "(cmu_us_awb_arctic::f0_clunit_name i)
Defines the unit name for unit selection for us.  The can be modified
changes the basic classification of unit for the clustering.  By default
this we just use the phone name, but you may want to make this, phone
plus previous phone (or something else)."
  (let ((name (item.name i)))
    (cond
      ((and (not cmu_us_awb_arctic::f0_clunits_loaded)
          (or (string-equal "h#" name) 
              (string-equal "1" (item.feat i "ignore"))
              (and (string-equal "pau" name)
                   (or (string-equal "pau" (item.feat i "p.name"))
                       (string-equal "h#" (item.feat i "p.name")))
                   (string-equal "pau" (item.feat i "n.name")))))
       "ignore")
     ((string-equal "pau" name)
      "pau")
     ((string-equal "-" (item.feat i "ph_cvox"))
      (string-append
      "unvoiced"
      "_"
      (item.feat i "ph_ctype")))
     (t
      (string-append 
       (item.feat i "R:SylStructure.parent.parent.R:Token.parent.EMPH")
       "_"
       (item.feat i "ph_vc")
       "_"
       (item.feat i "ph_vlng")
       "_"
       (item.feat i "ph_ctype")
;       "_"
;       (item.feat i "seg_coda_fric")
;       "_"
;       (item.feat i "seg_onset_stop")
       "_"
       (item.feat i "seg_onsetcoda")
;       "_"
;       (item.feat i "R:SylStructure.parent.accented")
       "_"
       (item.feat i "R:SylStructure.parent.stress")
       "_"
;       (if (< (item.feat i "R:SylStructure.parent.syl_break") 2) 
;	   0
;	   1)))
       (item.feat i "R:SylStructure.parent.syl_break")))
     )))


;;; These functions should probably be somewhere more generic

(define (unit_select_f0_model utt)

  ;; Setup clunit db for F0
  (set! spectral_db_clunits_params clunits_params)
  (set! clunits_params f0_db_clunits_params)
  (set! spectral_clunits_trees clunits_selection_trees)
  (set! clunits_selection_trees f0_clunits_trees)
  (clunits:select (get_param 'index_name clunits_params 'name))

  ;; Set up F0 database
  (Clunits_Select utt)
  (Clunits_Get_Units utt)

  ;; Collate selected F0 units into a single F0
  (Clunits_Join_F0_Units utt)

  ;; reinstate the normal clunits db
  (set! clunits_params spectral_db_clunits_params)
  (clunits:select (get_param 'index_name clunits_params 'name))
  (set! clunits_selection_trees spectral_clunits_trees)

  utt

)

(define (Clunits_Join_F0_Units utt)
  (let (;(ntmpdir (format nil "%s.f0" (make_tmp_filename)))
	(ntmpdir (format nil "%s.f0" "/tmp/est_f0clunits"))
	f0_track f0_item
	(n 1))

    (if (not (probe_file ntmpdir))
	(system (format nil "mkdir %s" ntmpdir)))
    (system (format nil "rm -rf %s/*" ntmpdir))
    (mapcar 
     (lambda (unit)
       (track.save
	(item.feat unit "coefs")
	(format nil "%s/f0_%05d.f0" ntmpdir n)
	"est_ascii")
	(set! n (+ 1 n)))
     (utt.relation.items utt 'Unit))

    (system (format 
	     nil 
	     "ch_track -otype est_ascii -o %s/all.f0 %s/f0_*.f0"
	     ntmpdir
	     ntmpdir))
    (set! f0_track (track.load (format nil "%s/all.f0" ntmpdir)))
    (utt.relation.create utt "f0")
    (set! f0_item (utt.relation.append utt "f0"))
    (item.set_feat f0_item "name" "f0")
    (item.set_feat f0_item "f0" f0_track)

    (utt.relation.delete utt "Unit")
    (utt.relation.delete utt "SourceSegments")

;    (system (format nil "rm -rf %s" ntmpdir))

    )
)

(define (is_pau i)
  (if (phone_is_silence (item.name i))
      "1"
      "0"))

(provide 'cmu_us_awb_arctic_f0clunits)

