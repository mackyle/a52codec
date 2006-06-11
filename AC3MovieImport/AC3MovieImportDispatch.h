/*
 *  AC3MovieImportDispatch.h
 *
 *    AC3Movie Import component dispatch helper header.
 *
 *
 *  Copyright (c) 2006  David Conrad
 *  Copyright (c) 2006  Graham Booker
 *
 *  This file is part of A52Codec.
 *
 *  A52Codec is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  A52Codec is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with A52Codec; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *
 */


        ComponentSelectorOffset (6)
	
	ComponentRangeCount (1)
	ComponentRangeShift (7)
	ComponentRangeMask  (7F)
	
	ComponentRangeBegin (0)
		ComponentError   (Target)
		ComponentError   (Register)
		StdComponentCall (Version)
		StdComponentCall (CanDo)
		StdComponentCall (Close)
		StdComponentCall (Open)
	ComponentRangeEnd (0)
	
	ComponentRangeBegin (1)
		ComponentError (0)
		ComponentError (Handle)
		ComponentCall  (File)
		ComponentError (SetSampleDuration)
		ComponentError (SetSampleDescription)
		ComponentError (SetMediaFile)
		ComponentError (SetDimensions)
		ComponentError  (SetChunkSize)
		ComponentError (SetProgressProc)
		ComponentError (SetAuxiliaryData)
		ComponentError (SetFromScrap)
		ComponentError (DoUserDialog)
		ComponentError (SetDuration)
		ComponentError (GetAuxiliaryDataType)
		ComponentCall  (Validate)
		ComponentCall  (GetFileType)
		ComponentCall  (DataRef)
		ComponentError (GetSampleDescription)
		ComponentCall  (GetMIMETypeList)
		ComponentCall  (SetOffsetAndLimit)
		ComponentError (GetSettingsAsAtomContainer)
		ComponentError (SetSettingsFromAtomContainer)
		ComponentCall  (SetOffsetAndLimit64)
		ComponentCall  (Idle)
		ComponentCall  (ValidateDataRef)
		ComponentError  (GetLoadState)
		ComponentError  (GetMaxLoadedTime)
		ComponentError  (EstimateCompletionTime)
		ComponentError  (SetDontBlock)
		ComponentError  (GetDontBlock)
		ComponentError  (SetIdleManager)
        ComponentError  (SetNewMovieFlags)
	ComponentRangeEnd (1)
